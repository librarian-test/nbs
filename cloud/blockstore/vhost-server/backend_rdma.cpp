#include "backend_rdma.h"

#include "backend.h"

#include <cloud/blockstore/libs/diagnostics/server_stats.h>
#include <cloud/blockstore/libs/rdma/impl/client.h>
#include <cloud/blockstore/libs/rdma/impl/verbs.h>
#include <cloud/blockstore/libs/service/request_helpers.h>
#include <cloud/blockstore/libs/service/storage.h>
#include <cloud/blockstore/libs/service/storage_provider.h>
#include <cloud/blockstore/libs/service_local/storage_rdma.h>
#include <cloud/blockstore/public/api/protos/volume.pb.h>
#include <cloud/contrib/vhost/include/vhost/server.h>
#include <cloud/storage/core/libs/common/task_queue.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>
#include <cloud/storage/core/libs/diagnostics/monitoring.h>

#include <library/cpp/protobuf/util/pb_io.h>
#include <library/cpp/uri/uri.h>

namespace NCloud::NBlockStore::NVHostServer {

using namespace NCloud::NBlockStore;

namespace {

////////////////////////////////////////////////////////////////////////////////

class TRdmaBackend final: public IBackend
{
private:
    const ILoggingServicePtr Logging;
    TLog Log;
    NRdma::IClientPtr RdmaClient;
    IStorageProviderPtr StorageProvider;
    IStoragePtr Storage;
    NProto::TVolume Volume;
    TString ClientId;
    ICompletionStatsPtr CompletionStats;
    TSimpleStats CompletionStatsData;
    bool ReadOnly;

public:
    explicit TRdmaBackend(ILoggingServicePtr logging);

    vhd_bdev_info Init(const TOptions& options) override;
    void Start() override;
    void Stop() override;
    void ProcessQueue(vhd_request_queue* queue, TSimpleStats& queueStats)
        override;
    std::optional<TSimpleStats> GetCompletionStats(TDuration timeout) override;

private:
    void ProcessReadRequest(struct vhd_io* io, TCpuCycles startCycles);
    void ProcessWriteRequest(struct vhd_io* io, TCpuCycles startCycles);
    void CompleteRequest(
        struct vhd_io* io,
        TCpuCycles startCycles,
        bool isError);
};

////////////////////////////////////////////////////////////////////////////////

TRdmaBackend::TRdmaBackend(ILoggingServicePtr logging)
    : Logging{std::move(logging)}
    , CompletionStats(CreateCompletionStats())
{
    Log = Logging->CreateLog("RDMA");
}

std::tuple<TString, ui16, TString> ParseRdmaDevicePath(
    const TString& devicePath)
{
    // rdma://myt1-ct5-13.cloud.yandex.net:10020/62ccf40f2743308191205ad6391bfa06

    NUri::TUri uri;
    auto res = uri.Parse(
        devicePath,
        NUri::TFeature::FeaturesDefault | NUri::TFeature::FeatureSchemeFlexible);
    Y_ENSURE(
        res == NUri::TState::ParsedOK,
        "invalid device path " << devicePath);

    Y_ENSURE(uri.GetField(NUri::TField::FieldScheme) == "rdma",
        "device path doesn't start with rdma://, " << devicePath);

    auto path = uri.GetField(NUri::TField::FieldPath);
    Y_ENSURE(
        path.size() > 1 && path[0] == '/',
        "invalid uuid inside device path " << devicePath);

    auto uuid = path.substr(1);

    return {TString{uri.GetHost()}, uri.GetPort(), TString{uuid}};
}

vhd_bdev_info TRdmaBackend::Init(const TOptions& options)
{
    STORAGE_INFO("Initializing RDMA backend");

    ClientId = options.ClientId;
    ReadOnly = options.ReadOnly;

    auto rdmaClientConfig = std::make_shared<NRdma::TClientConfig>();
    rdmaClientConfig->QueueSize = options.RdmaClient.QueueSize;
    rdmaClientConfig->MaxBufferSize = options.RdmaClient.MaxBufferSize;

    RdmaClient = NRdma::CreateClient(
        NRdma::NVerbs::CreateVerbs(),
        Logging,
        NCloud::CreateMonitoringServiceStub(),
        std::move(rdmaClientConfig));

    StorageProvider = NStorage::CreateRdmaStorageProvider(
        CreateServerStatsStub(),
        RdmaClient,
        NStorage::ERdmaTaskQueueOpt::DontUse);

    Volume.SetStorageMediaKind(NProto::STORAGE_MEDIA_SSD_NONREPLICATED);
    Volume.SetBlockSize(VHD_SECTOR_SIZE);
    Volume.SetDiskId(options.DiskId);

    ui64 totalBytes = 0;

    for (auto& chunk: options.Layout) {
        auto [host, port, uuid] = ParseRdmaDevicePath(chunk.DevicePath);
        auto* device = Volume.MutableDevices()->Add();
        device->SetDeviceUUID(uuid);
        device->MutableRdmaEndpoint()->SetHost(host);
        device->MutableRdmaEndpoint()->SetPort(port);

        Y_ENSURE(
            chunk.Offset == 0,
            "device chunk offset is not 0"
                << ", device=" << chunk.DevicePath
                << ", offset=" << chunk.Offset);

        Y_ENSURE(
            chunk.ByteCount % Volume.GetBlockSize() == 0,
            "device chunk size is not aligned to "
                << Volume.GetBlockSize()
                << ", device=" << chunk.DevicePath
                << ", byte_count=" << chunk.ByteCount);

        device->SetBlockCount(chunk.ByteCount / Volume.GetBlockSize());
        totalBytes += chunk.ByteCount;
    }

    STORAGE_INFO("Volume:"
        << " DiskId=" << Volume.GetDiskId()
        << " TotalBlocks=" << totalBytes / Volume.GetBlockSize()
        << " BlockSize=" << Volume.GetBlockSize());

    return {
        .serial = options.Serial.c_str(),
        .socket_path = options.SocketPath.c_str(),
        .block_size = Volume.GetBlockSize(),
        .num_queues = options.QueueCount,   // Max count of virtio queues
        .total_blocks = totalBytes / Volume.GetBlockSize(),
        .features = ReadOnly ? VHD_BDEV_F_READONLY : 0};
}

void TRdmaBackend::Start()
{
    STORAGE_INFO("Starting RDMA backend");

    RdmaClient->Start();

    auto accessMode = ReadOnly ? NProto::VOLUME_ACCESS_READ_ONLY
                               : NProto::VOLUME_ACCESS_READ_WRITE;
    auto future = StorageProvider->CreateStorage(
        Volume,
        ClientId,
        accessMode);
    Storage = future.GetValueSync();
}

void TRdmaBackend::Stop()
{
    STORAGE_INFO("Stopping RDMA backend");

    RdmaClient->Stop();
}

void TRdmaBackend::ProcessQueue(
    vhd_request_queue* queue,
    TSimpleStats& queueStats)
{
    vhd_request req;
    while (vhd_dequeue_request(queue, &req)) {
        ++queueStats.Dequeued;

        struct vhd_bdev_io* bio = vhd_get_bdev_io(req.io);
        const TCpuCycles now = GetCycleCount();
        switch (bio->type) {
            case VHD_BDEV_READ:
                ProcessReadRequest(req.io, now);
                ++queueStats.Submitted;
                break;
            case VHD_BDEV_WRITE:
                ProcessWriteRequest(req.io, now);
                ++queueStats.Submitted;
                break;
            default:
                STORAGE_ERROR(
                    "Unexpected vhost request type: "
                    << static_cast<int>(bio->type));
                vhd_complete_bio(req.io, VHD_BDEV_IOERR);
                ++queueStats.SubFailed;
                break;
        }
    }
}

std::optional<TSimpleStats> TRdmaBackend::GetCompletionStats(TDuration timeout)
{
    return CompletionStats->Get(timeout);
}

TSgList ConvertVhdSgList(const vhd_sglist& vhdSglist)
{
    TSgList sgList;
    sgList.reserve(vhdSglist.nbuffers);
    for (ui32 i = 0; i < vhdSglist.nbuffers; ++i) {
        const auto& buffer = vhdSglist.buffers[i];
        sgList.emplace_back(static_cast<char*>(buffer.base), buffer.len);
    }
    return sgList;
}

void TRdmaBackend::ProcessReadRequest(struct vhd_io* io, TCpuCycles startCycles)
{
    auto* bio = vhd_get_bdev_io(io);

    auto request = std::make_shared<NProto::TReadBlocksLocalRequest>();
    auto requestId = CreateRequestId();
    auto callContext = MakeIntrusive<TCallContext>(requestId);

    auto* reqHeaders = request->MutableHeaders();
    reqHeaders->SetRequestId(requestId);
    reqHeaders->SetTimestamp(TInstant::Now().MicroSeconds());
    reqHeaders->SetClientId(ClientId);

    request->SetStartIndex(bio->first_sector);
    request->SetBlocksCount(bio->total_sectors);
    request->BlockSize = Volume.GetBlockSize();
    request->Sglist.SetSgList(std::move(ConvertVhdSgList(bio->sglist)));

    STORAGE_DEBUG(
        "READ[%lu] Index=%lu, BlocksCount=%d, BlockSize=%d",
        reqHeaders->GetRequestId(),
        request->GetStartIndex(),
        request->GetBlocksCount(),
        request->BlockSize);
    auto future =
        Storage->ReadBlocksLocal(std::move(callContext), std::move(request));
    future.Subscribe(
        [this, io, requestId, startCycles](const auto& future)
        {
            const auto& response = future.GetValue();
            auto& error = response.GetError();
            STORAGE_DEBUG(
                "READ[%lu] Code=%d, Message=%s",
                requestId,
                error.GetCode(),
                error.GetMessage().c_str());
            CompleteRequest(io, startCycles, HasError(error));
        });
}

void TRdmaBackend::ProcessWriteRequest(
    struct vhd_io* io,
    TCpuCycles startCycles)
{
    auto* bio = vhd_get_bdev_io(io);

    auto request = std::make_shared<NProto::TWriteBlocksLocalRequest>();
    auto requestId = CreateRequestId();
    auto callContext = MakeIntrusive<TCallContext>(requestId);

    auto* reqHeaders = request->MutableHeaders();
    reqHeaders->SetRequestId(requestId);
    reqHeaders->SetTimestamp(TInstant::Now().MicroSeconds());
    reqHeaders->SetClientId(ClientId);

    request->SetStartIndex(bio->first_sector);
    request->BlocksCount = bio->total_sectors;
    request->BlockSize = Volume.GetBlockSize();
    request->Sglist.SetSgList(std::move(ConvertVhdSgList(bio->sglist)));

    STORAGE_DEBUG(
        "WRITE[%lu] Index=%lu, BlocksCount=%d, BlockSize=%d",
        reqHeaders->GetRequestId(),
        request->GetStartIndex(),
        request->BlocksCount,
        request->BlockSize);
    auto future =
        Storage->WriteBlocksLocal(std::move(callContext), std::move(request));
    future.Subscribe(
        [this, io, requestId, startCycles](const auto& future)
        {
            const auto& response = future.GetValue();
            auto& error = response.GetError();
            STORAGE_DEBUG(
                "WRITE[%lu] Code=%d, Message=%s",
                requestId,
                error.GetCode(),
                error.GetMessage().c_str());
            CompleteRequest(io, startCycles, HasError(error));
        });
}

void TRdmaBackend::CompleteRequest(
    struct vhd_io* io,
    TCpuCycles startCycles,
    bool isError)
{
    auto* bio = vhd_get_bdev_io(io);

    ++CompletionStatsData.Completed;

    if (!isError) {
        const ui64 bytes = bio->total_sectors * VHD_SECTOR_SIZE;
        CompletionStatsData.Requests[bio->type].Count += 1;
        CompletionStatsData.Requests[bio->type].Bytes += bytes;
        CompletionStatsData.Sizes[bio->type].Increment(bytes);
        CompletionStatsData.Times[bio->type].Increment(
            GetCycleCount() - startCycles);
    } else {
        CompletionStatsData.Requests[bio->type].Errors += 1;
    }

    vhd_complete_bio(io, isError ? VHD_BDEV_IOERR : VHD_BDEV_SUCCESS);

    CompletionStats->Sync(CompletionStatsData);
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IBackendPtr CreateRdmaBackend(ILoggingServicePtr logging)
{
    return std::make_shared<TRdmaBackend>(std::move(logging));
}

}   // namespace NCloud::NBlockStore::NVHostServer
