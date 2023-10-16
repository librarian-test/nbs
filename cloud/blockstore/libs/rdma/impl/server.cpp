#include "server.h"

#include "buffer.h"
#include "error.h"
#include "event.h"
#include "verbs.h"
#include "work_queue.h"
#include "adaptive_wait.h"

#include <cloud/blockstore/libs/rdma/iface/error.h>
#include <cloud/blockstore/libs/rdma/iface/list.h>
#include <cloud/blockstore/libs/rdma/iface/poll.h>
#include <cloud/blockstore/libs/rdma/iface/probes.h>
#include <cloud/blockstore/libs/rdma/iface/protocol.h>
#include <cloud/blockstore/libs/rdma/iface/rcu.h>
#include <cloud/blockstore/libs/rdma/iface/utils.h>

#include <cloud/blockstore/libs/service/context.h>
#include <cloud/blockstore/libs/diagnostics/critical_events.h>

#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/common/thread.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>
#include <cloud/storage/core/libs/diagnostics/monitoring.h>

#include <library/cpp/monlib/dynamic_counters/counters.h>

#include <util/datetime/base.h>
#include <util/generic/vector.h>
#include <util/random/random.h>
#include <util/system/mutex.h>
#include <util/system/thread.h>

namespace NCloud::NBlockStore::NRdma {

using namespace NMonitoring;

LWTRACE_USING(BLOCKSTORE_RDMA_PROVIDER);

namespace {

////////////////////////////////////////////////////////////////////////////////

constexpr TDuration LOG_THROTTLER_PERIOD = TDuration::Minutes(20);
constexpr TDuration POLL_TIMEOUT = TDuration::Seconds(1);

////////////////////////////////////////////////////////////////////////////////

struct TRequest;
using TRequestPtr = std::unique_ptr<TRequest>;

struct TEndpointCounters;
using TEndpointCountersPtr = std::shared_ptr<TEndpointCounters>;

class TServerSession;
using TServerSessionPtr = std::shared_ptr<TServerSession>;

class TServerEndpoint;
using TServerEndpointPtr = std::shared_ptr<TServerEndpoint>;

class TConnectionPoller;
using TConnectionPollerPtr = std::unique_ptr<TConnectionPoller>;

class TCompletionPoller;
using TCompletionPollerPtr = std::unique_ptr<TCompletionPoller>;

////////////////////////////////////////////////////////////////////////////////

enum class ERequestState
{
    RecvRequest,
    ReadRequestData,
    ExecuteRequest,
    WriteResponseData,
    SendResponse,
};

////////////////////////////////////////////////////////////////////////////////

struct TRequest
    : TListNode<TRequest>
{
    std::weak_ptr<TServerSession> Session;

    ERequestState State = ERequestState::RecvRequest;

    TCallContextPtr CallContext;
    ui32 ReqId = 0;
    ui32 Status = 0;
    ui32 ResponseBytes = 0;

    TBufferDesc In {};
    TBufferDesc Out {};

    TPooledBuffer InBuffer {};
    TPooledBuffer OutBuffer {};

    TRequest(std::weak_ptr<TServerSession> session)
        : Session(std::move(session))
    {}
};

////////////////////////////////////////////////////////////////////////////////

void StoreRequest(TSendWr* send, TRequestPtr req)
{
    Y_ABORT_UNLESS(send->context == nullptr);
    send->context = req.release();  // ownership transferred
}

TRequestPtr ExtractRequest(TSendWr* send)
{
    TRequestPtr req(static_cast<TRequest*>(send->context));
    send->context = nullptr;
    return req;
}

////////////////////////////////////////////////////////////////////////////////

struct TEndpointCounters
{
    TDynamicCounters::TCounterPtr QueuedRequests;
    TDynamicCounters::TCounterPtr ActiveRequests;
    TDynamicCounters::TCounterPtr CompletedRequests;
    TDynamicCounters::TCounterPtr ThrottledRequests;

    TDynamicCounters::TCounterPtr ActiveSend;
    TDynamicCounters::TCounterPtr ActiveRecv;
    TDynamicCounters::TCounterPtr ActiveRead;
    TDynamicCounters::TCounterPtr ActiveWrite;

    TDynamicCounters::TCounterPtr SendErrors;
    TDynamicCounters::TCounterPtr RecvErrors;
    TDynamicCounters::TCounterPtr ReadErrors;
    TDynamicCounters::TCounterPtr WriteErrors;

    void Register(TDynamicCounters& counters)
    {
        QueuedRequests = counters.GetCounter("QueuedRequests");
        ActiveRequests = counters.GetCounter("ActiveRequests");
        CompletedRequests = counters.GetCounter("CompletedRequests", true);
        ThrottledRequests = counters.GetCounter("ThrottledRequests", true);

        ActiveSend = counters.GetCounter("ActiveSend");
        ActiveRecv = counters.GetCounter("ActiveRecv");
        ActiveRead = counters.GetCounter("ActiveRead");
        ActiveWrite = counters.GetCounter("ActiveWrite");

        SendErrors = counters.GetCounter("SendErrors");
        RecvErrors = counters.GetCounter("RecvErrors");
        ReadErrors = counters.GetCounter("ReadErrors");
        WriteErrors = counters.GetCounter("WriteErrors");
    }

    void RequestEnqueued()
    {
        QueuedRequests->Inc();
    }

    void RequestDequeued()
    {
        QueuedRequests->Dec();
    }

    void RecvRequestStarted()
    {
        ActiveRecv->Inc();
    }

    void RecvRequestCompleted()
    {
        ActiveRecv->Dec();
        ActiveRequests->Inc();
    }

    void ReadRequestStarted()
    {
        ActiveRead->Inc();
    }

    void ReadRequestCompleted()
    {
        ActiveRead->Dec();
    }

    void WriteResponseStarted()
    {
        ActiveWrite->Inc();
    }

    void WriteResponseCompleted()
    {
        ActiveWrite->Dec();
    }

    void SendResponseStarted()
    {
        ActiveSend->Inc();
    }

    void SendResponseCompleted()
    {
        ActiveSend->Dec();
        ActiveRequests->Dec();
        CompletedRequests->Inc();
    }

    void RecvRequestError()
    {
        ActiveRecv->Dec();
        RecvErrors->Inc();
    }

    void ReadRequestError()
    {
        ActiveRead->Dec();
        ActiveRequests->Dec();
        ReadErrors->Inc();
    }

    void WriteResponseError()
    {
        ActiveWrite->Dec();
        ActiveRequests->Dec();
        WriteErrors->Inc();
    }

    void SendResponseError()
    {
        ActiveSend->Dec();
        ActiveRequests->Dec();
        SendErrors->Inc();
    }

    void RequestThrottled()
    {
        ActiveSend->Dec();
        ActiveRequests->Dec();
        ThrottledRequests->Inc();
    }
};

////////////////////////////////////////////////////////////////////////////////

class TServerSession final
    : public NVerbs::ICompletionHandler
    , public std::enable_shared_from_this<TServerSession>
{
    // TODO
    friend class TServer;
    friend class TCompletionPoller;

private:
    NVerbs::IVerbsPtr Verbs;
    NVerbs::TConnectionPtr Connection;
    TCompletionPoller* CompletionPoller = nullptr;
    IServerHandlerPtr Handler;
    TServerConfigPtr Config;
    TEndpointCountersPtr Counters;
    TLog Log;
    size_t MaxInflightBytes;

    TLogThrottler LogThrottler = TLogThrottler(LOG_THROTTLER_PERIOD);

    NVerbs::TCompletionChannelPtr CompletionChannel = NVerbs::NullPtr;
    NVerbs::TCompletionQueuePtr CompletionQueue = NVerbs::NullPtr;

    TBufferPool SendBuffers;
    TBufferPool RecvBuffers;

    TPooledBuffer SendBuffer {};
    TPooledBuffer RecvBuffer {};

    TVector<TSendWr> SendWrs;
    TVector<TRecvWr> RecvWrs;

    TWorkQueue<TSendWr> SendQueue;
    TWorkQueue<TRecvWr> RecvQueue;

    TLockFreeList<TRequest> InputRequests;
    TSimpleList<TRequest> QueuedRequests;
    TEventHandle RequestEvent;

public:
    static TServerSession* FromEvent(rdma_cm_event* event)
    {
        Y_ABORT_UNLESS(event->id && event->id->context);
        return static_cast<TServerSession*>(event->id->context);
    }

    TServerSession(
        NVerbs::IVerbsPtr Verbs,
        NVerbs::TConnectionPtr connection,
        TCompletionPoller* completionPoller,
        IServerHandlerPtr handler,
        TServerConfigPtr config,
        TEndpointCountersPtr stats,
        TLog log);

    ~TServerSession();

    // called from CM thread
    void Start();
    void Stop() noexcept;

    // called from external thread
    void EnqueueRequest(TRequestPtr req) noexcept;

    // called from CQ thread
    bool HandleInputRequests();
    bool HandleCompletionEvents();

private:
    // called inderectly from CQ by 2 previous functions
    void HandleQueuedRequests();
    void HandleCompletionEvent(ibv_wc* wc) override;
    void RecvRequest(TRecvWr* recv);
    void RecvRequestCompleted(TRecvWr* recv, ibv_wc_status status);
    void ReadRequestData(TRequestPtr req, TSendWr* send);
    void ReadRequestDataCompleted(TSendWr* send, ibv_wc_status status);
    void ExecuteRequest(TRequestPtr req);
    void WriteResponseData(TRequestPtr req, TSendWr* send);
    void WriteResponseDataCompleted(TSendWr* send, ibv_wc_status status);
    void SendResponse(TRequestPtr req, TSendWr* send);
    void SendResponseCompleted(TSendWr* send, ibv_wc_status status);
    void FreeRequest(TRequestPtr req, TSendWr* send) noexcept;
    void RejectRequest(TRequestPtr req, ui32 status, TStringBuf message);
};

////////////////////////////////////////////////////////////////////////////////

TServerSession::TServerSession(
        NVerbs::IVerbsPtr verbs,
        NVerbs::TConnectionPtr connection,
        TCompletionPoller* completionPoller,
        IServerHandlerPtr handler,
        TServerConfigPtr config,
        TEndpointCountersPtr stats,
        TLog log)
    : Verbs(std::move(verbs))
    , Connection(std::move(connection))
    , CompletionPoller(completionPoller)
    , Handler(std::move(handler))
    , Config(std::move(config))
    , Counters(std::move(stats))
    , Log(log)
    , MaxInflightBytes(Config->MaxInflightBytes)
{
    Connection->context = this;

    CompletionChannel = Verbs->CreateCompletionChannel(Connection->verbs);
    SetNonBlock(CompletionChannel->fd, true);

    CompletionQueue = Verbs->CreateCompletionQueue(
        Connection->verbs,
        2 * Config->QueueSize,   // send + recv
        this,
        CompletionChannel.get(),
        0);                      // comp_vector

    ibv_qp_init_attr qp_attrs = {
        .qp_context = nullptr,
        .send_cq = CompletionQueue.get(),
        .recv_cq = CompletionQueue.get(),
        .cap = {
            .max_send_wr = Config->QueueSize,
            .max_recv_wr = Config->QueueSize,
            .max_send_sge = RDMA_MAX_SEND_SGE,
            .max_recv_sge = RDMA_MAX_RECV_SGE,
            .max_inline_data = 16,
        },
        .qp_type = IBV_QPT_RC,
        .sq_sig_all = 1,
    };

    Verbs->CreateQP(Connection.get(), &qp_attrs);

    SendBuffers.Init(
        Verbs,
        Connection->pd,
        IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ);

    RecvBuffers.Init(
        Verbs,
        Connection->pd,
        IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE);

    SendBuffer = SendBuffers.AcquireBuffer(
        Config->QueueSize * sizeof(TResponseMessage),
        true);

    RecvBuffer = RecvBuffers.AcquireBuffer(
        Config->QueueSize * sizeof(TRequestMessage),
        true);

    SendWrs.resize(Config->QueueSize);
    RecvWrs.resize(Config->QueueSize);

    ui64 responseMsg = SendBuffer.Address;
    for (auto& wr: SendWrs) {
        wr.wr.opcode = IBV_WR_SEND;

        wr.wr.wr_id = reinterpret_cast<uintptr_t>(&wr);
        wr.wr.sg_list = wr.sg_list;
        wr.wr.num_sge = 1;

        wr.sg_list[0].lkey = SendBuffer.Key;
        wr.sg_list[0].addr = responseMsg;
        wr.sg_list[0].length = sizeof(TResponseMessage);

        SendQueue.Push(&wr);
        responseMsg += sizeof(TResponseMessage);
    }

    ui64 requestMsg = RecvBuffer.Address;
    for (auto& wr: RecvWrs) {
        wr.wr.wr_id = reinterpret_cast<uintptr_t>(&wr);
        wr.wr.sg_list = wr.sg_list;
        wr.wr.num_sge = 1;

        wr.sg_list[0].lkey = RecvBuffer.Key;
        wr.sg_list[0].addr = requestMsg;
        wr.sg_list[0].length = sizeof(TRequestMessage);

        RecvQueue.Push(&wr);
        requestMsg += sizeof(TRequestMessage);
    }
}

TServerSession::~TServerSession()
{
    STORAGE_DEBUG("destroy client session");

    Verbs->DestroyQP(Connection.get());

    CompletionQueue.reset();
    CompletionChannel.reset();

    if (SendBuffers.Initialized()) {
        SendBuffers.ReleaseBuffer(SendBuffer);
    }

    if (RecvBuffers.Initialized()) {
        RecvBuffers.ReleaseBuffer(RecvBuffer);
    }

    SendQueue.Clear();
    RecvQueue.Clear();
}

void TServerSession::Start()
{
    while (auto* recv = RecvQueue.Pop()) {
        RecvRequest(recv);
    }
}

void TServerSession::Stop() noexcept
{
    try {
        Verbs->Disconnect(Connection.get());

    } catch (const TServiceError &e) {
        STORAGE_ERROR("unable to disconnect session");
    }
}

void TServerSession::EnqueueRequest(TRequestPtr req) noexcept
{
    Counters->RequestEnqueued();
    InputRequests.Enqueue(std::move(req));

    if (Config->WaitMode == EWaitMode::Poll) {
        RequestEvent.Set();
    }
}

bool TServerSession::HandleInputRequests()
{
    if (Config->WaitMode == EWaitMode::Poll) {
        RequestEvent.Clear();
    }

    auto requests = InputRequests.DequeueAll();
    if (!requests) {
        return false;
    }

    QueuedRequests.Append(std::move(requests));
    HandleQueuedRequests();
    return true;
}

void TServerSession::HandleQueuedRequests()
{
    while (QueuedRequests) {
        auto* send = SendQueue.Pop();
        if (!send) {
            // no more WRs available
            break;
        }

        auto req = QueuedRequests.Dequeue();
        Y_ABORT_UNLESS(req);

        Counters->RequestDequeued();

        switch (req->State) {
            case ERequestState::RecvRequest:
                Y_ABORT_UNLESS(req->In.Length);
                ReadRequestData(std::move(req), send);
                break;

            case ERequestState::ExecuteRequest:
                if (req->ResponseBytes) {
                    WriteResponseData(std::move(req), send);
                } else {
                    SendResponse(std::move(req), send);
                }
                break;

            default:
                Y_ABORT("unexpected state: %d", (int)req->State);
        }
    }
}

bool TServerSession::HandleCompletionEvents()
{
    bool hasWork = false;

    ibv_cq* cq = CompletionQueue.get();

    if (Config->WaitMode == EWaitMode::Poll) {
        Verbs->GetCompletionEvent(cq);
        Verbs->AckCompletionEvents(cq, 1);
        Verbs->RequestCompletionEvent(cq, 0);
    }

    hasWork = Verbs->PollCompletionQueue(cq, this);

    HandleQueuedRequests();

    return hasWork;
}

void TServerSession::HandleCompletionEvent(ibv_wc* wc)
{
    STORAGE_TRACE(NVerbs::GetOpcodeName(wc->opcode) << " #" << wc->wr_id
        << " completed");

    // flush means we are either closing or already closed this session
    if (wc->status == IBV_WC_WR_FLUSH_ERR) {
        return;
    }

    switch (wc->opcode) {
        case IBV_WC_RECV:
            RecvRequestCompleted(
                reinterpret_cast<TRecvWr*>(wc->wr_id),
                wc->status);
            break;

        case IBV_WC_RDMA_READ:
            ReadRequestDataCompleted(
                reinterpret_cast<TSendWr*>(wc->wr_id),
                wc->status);
            break;

        case IBV_WC_RDMA_WRITE:
            WriteResponseDataCompleted(
                reinterpret_cast<TSendWr*>(wc->wr_id),
                wc->status);
            break;

        case IBV_WC_SEND:
            SendResponseCompleted(
                reinterpret_cast<TSendWr*>(wc->wr_id),
                wc->status);
            break;

        default:
            STORAGE_ERROR("unexpected completion: "
                << NVerbs::PrintCompletion(wc));
    }
}

void TServerSession::RecvRequest(TRecvWr* recv)
{
    auto* requestMsg = recv->Message<TRequestMessage>();
    Zero(*requestMsg);

    STORAGE_TRACE("RECV #" << recv->wr.wr_id);
    Verbs->PostRecv(Connection->qp, &recv->wr);
    Counters->RecvRequestStarted();
}

void TServerSession::FreeRequest(TRequestPtr req, TSendWr* send) noexcept
{
    RecvBuffers.ReleaseBuffer(req->InBuffer);
    SendBuffers.ReleaseBuffer(req->OutBuffer);
    MaxInflightBytes += req->In.Length + req->Out.Length;

    SendQueue.Push(send);
}

void TServerSession::RecvRequestCompleted(TRecvWr* recv, ibv_wc_status status)
{
    if (status == IBV_WC_WR_FLUSH_ERR) {
        RecvQueue.Push(recv);;
        return;
    }

    if (status != IBV_WC_SUCCESS) {
        STORAGE_ERROR("RECV #" << recv->wr.wr_id << ": "
            << NVerbs::GetStatusString(status));

        Counters->RecvRequestError();
        ReportRdmaError();
        RecvRequest(recv);  // should always be posted
        return;
    }

    const auto* msg = recv->Message<TRequestMessage>();
    const int version = ParseMessageHeader(msg);

    if (version != RDMA_PROTO_VERSION) {
        STORAGE_ERROR("RECV #" << recv->wr.wr_id
            << ": incompatible protocol version "
            << version << " != "<< int(RDMA_PROTO_VERSION));

        Counters->RecvRequestError();
        ReportRdmaError();
        RecvRequest(recv);  // should always be posted
        return;
    }

    auto req = std::make_unique<TRequest>(weak_from_this());

    // TODO restore context from request meta
    req->CallContext = MakeIntrusive<TCallContext>();
    req->ReqId = msg->ReqId;
    req->In = msg->In;
    req->Out = msg->Out;

    Counters->RecvRequestCompleted();
    RecvRequest(recv);  // should always be posted

    if (req->In.Length > Config->MaxBufferSize) {
        STORAGE_ERROR("RECV #" << recv->wr.wr_id
            << ": request exceeds maximum supported size "
            << req->In.Length << " > " << Config->MaxBufferSize);

        Counters->RecvRequestError();
        return;
    }

    if (req->Out.Length > Config->MaxBufferSize) {
        STORAGE_ERROR("RECV #" << recv->wr.wr_id
            << ": request exceeds maximum supported size "
            << req->Out.Length << " > " << Config->MaxBufferSize);

        Counters->RecvRequestError();
        return;
    }

    LWTRACK(
        RecvRequestCompleted,
        req->CallContext->LWOrbit,
        req->CallContext->RequestId);

    if (MaxInflightBytes < req->In.Length + req->Out.Length) {
        STORAGE_INFO_T(LogThrottler, "reached inflight limit, "
            << MaxInflightBytes << "/" << Config->MaxInflightBytes
            << " bytes available");

        return RejectRequest(std::move(req), RDMA_PROTO_THROTTLED,
            "throttled");
    }

    MaxInflightBytes -= req->In.Length + req->Out.Length;

    if (req->Out.Length) {
        req->OutBuffer = SendBuffers.AcquireBuffer(req->Out.Length);
    }

    if (req->In.Length) {
        req->InBuffer = RecvBuffers.AcquireBuffer(req->In.Length);

        if (auto* send = SendQueue.Pop()) {
            ReadRequestData(std::move(req), send);
        } else {
            // no more WRs available
            Counters->RequestEnqueued();
            QueuedRequests.Enqueue(std::move(req));
        }
    } else {
        ExecuteRequest(std::move(req));
    }
}

void TServerSession::RejectRequest(
    TRequestPtr req,
    ui32 status,
    TStringBuf message)
{
    req->State = ERequestState::ExecuteRequest;
    req->Status = status;
    req->In.Length = 0;
    req->Out.Length = 1_KB;
    req->OutBuffer = SendBuffers.AcquireBuffer(req->Out.Length);
    req->ResponseBytes = SerializeError(
        E_REJECTED,
        message,
        static_cast<TStringBuf>(req->OutBuffer));

    MaxInflightBytes -= req->Out.Length;

    if (auto* send = SendQueue.Pop()) {
        WriteResponseData(std::move(req), send);
    } else {
        // no more WRs available
        Counters->RequestEnqueued();
        QueuedRequests.Enqueue(std::move(req));
    }
}

void TServerSession::ReadRequestData(TRequestPtr req, TSendWr* send)
{
    req->State = ERequestState::ReadRequestData;

    ibv_sge sg_list = {
        .addr = req->InBuffer.Address,
        .length = req->In.Length,
        .lkey = req->InBuffer.Key,
    };

    ibv_send_wr wr = {
        .wr_id = reinterpret_cast<uintptr_t>(send),
        .sg_list = &sg_list,
        .num_sge = 1,
        .opcode = IBV_WR_RDMA_READ,
    };

    wr.wr.rdma.rkey = req->In.Key;
    wr.wr.rdma.remote_addr = req->In.Address;

    STORAGE_TRACE("READ #" << wr.wr_id);
    Verbs->PostSend(Connection->qp, &wr);
    Counters->ReadRequestStarted();

    LWTRACK(
        ReadRequestDataStarted,
        req->CallContext->LWOrbit,
        req->CallContext->RequestId);

    StoreRequest(send, std::move(req));
}

void TServerSession::ReadRequestDataCompleted(
    TSendWr* send,
    ibv_wc_status status)
{
    auto req = ExtractRequest(send);

    if (req == nullptr) {
        STORAGE_WARN("READ #" << send->wr.wr_id << ": request is empty");
        return;
    }

    if (status != IBV_WC_SUCCESS) {
        STORAGE_ERROR("READ #" << send->wr.wr_id << ": "
            << NVerbs::GetStatusString(status));

        Counters->ReadRequestError();
        ReportRdmaError();
        FreeRequest(std::move(req), send);
        return;
    }

    Y_ABORT_UNLESS(req->State == ERequestState::ReadRequestData);

    SendQueue.Push(send);
    Counters->ReadRequestCompleted();

    LWTRACK(
        ReadRequestDataCompleted,
        req->CallContext->LWOrbit,
        req->CallContext->RequestId);

    ExecuteRequest(std::move(req));
}

void TServerSession::ExecuteRequest(TRequestPtr req)
{
    req->State = ERequestState::ExecuteRequest;

    TStringBuf in = {
        reinterpret_cast<char*>(req->InBuffer.Address),
        req->In.Length,
    };

    TStringBuf out = {
        reinterpret_cast<char*>(req->OutBuffer.Address),
        req->Out.Length,
    };

    LWTRACK(
        ExecuteRequest,
        req->CallContext->LWOrbit,
        req->CallContext->RequestId);

    Handler->HandleRequest(req.get(), req->CallContext, in, out);

    req.release();  // ownership transferred
}

void TServerSession::WriteResponseData(TRequestPtr req, TSendWr* send)
{
    req->State = ERequestState::WriteResponseData;

    ibv_sge sg_list = {
        .addr = req->OutBuffer.Address,
        .length = req->ResponseBytes,
        .lkey = req->OutBuffer.Key,
    };

    ibv_send_wr wr = {
        .wr_id = reinterpret_cast<uintptr_t>(send),
        .sg_list = &sg_list,
        .num_sge = 1,
        .opcode = IBV_WR_RDMA_WRITE,
    };

    wr.wr.rdma.rkey = req->Out.Key;
    wr.wr.rdma.remote_addr = req->Out.Address;

    STORAGE_TRACE("WRITE #" << wr.wr_id);
    Verbs->PostSend(Connection->qp, &wr);

    LWTRACK(
        WriteResponseDataStarted,
        req->CallContext->LWOrbit,
        req->CallContext->RequestId);

    StoreRequest(send, std::move(req));
}

void TServerSession::WriteResponseDataCompleted(
    TSendWr* send,
    ibv_wc_status status)
{
    auto req = ExtractRequest(send);

    if (req == nullptr) {
        STORAGE_WARN("WRITE #" << send->wr.wr_id << ": request is empty");
        return;
    }

    if (status != IBV_WC_SUCCESS) {
        if (status != IBV_WC_SUCCESS) {
            STORAGE_ERROR("WRITE #" << send->wr.wr_id << ": "
                << NVerbs::GetStatusString(status));
        }

        Counters->WriteResponseError();
        ReportRdmaError();
        FreeRequest(std::move(req), send);
        return;
    }

    Y_ABORT_UNLESS(req->State == ERequestState::WriteResponseData);

    Counters->WriteResponseCompleted();

    LWTRACK(
        WriteResponseDataCompleted,
        req->CallContext->LWOrbit,
        req->CallContext->RequestId);

    SendResponse(std::move(req), send);
}

void TServerSession::SendResponse(TRequestPtr req, TSendWr* send)
{
    req->State = ERequestState::SendResponse;

    auto* responseMsg = send->Message<TResponseMessage>();
    Zero(*responseMsg);

    InitMessageHeader(responseMsg, RDMA_PROTO_VERSION);

    responseMsg->ReqId = req->ReqId;
    responseMsg->Status = req->Status;
    responseMsg->ResponseBytes = req->ResponseBytes;

    STORAGE_TRACE("SEND #" << send->wr.wr_id);
    Verbs->PostSend(Connection->qp, &send->wr);
    Counters->SendResponseStarted();

    LWTRACK(
        SendResponseStarted,
        req->CallContext->LWOrbit,
        req->CallContext->RequestId);

    StoreRequest(send, std::move(req));
}

void TServerSession::SendResponseCompleted(TSendWr* send, ibv_wc_status status)
{
    if (status == IBV_WC_WR_FLUSH_ERR) {
        SendQueue.Push(send);
        return;
    }

    auto req = ExtractRequest(send);

    if (req == nullptr) {
        STORAGE_WARN("SEND #" << send->wr.wr_id << ": request is empty");
        return;
    }

    if (status != IBV_WC_SUCCESS) {
        STORAGE_ERROR("SEND #" << send->wr.wr_id << ": "
            << NVerbs::GetStatusString(status));

        Counters->SendResponseError();
        ReportRdmaError();
        FreeRequest(std::move(req), send);
        return;
    }

    Y_ABORT_UNLESS(req->State == ERequestState::SendResponse);

    if (req->Status == RDMA_PROTO_THROTTLED) {
        Counters->RequestThrottled();
    } else {
        Counters->SendResponseCompleted();
    }

    LWTRACK(
        SendResponseCompleted,
        req->CallContext->LWOrbit,
        req->CallContext->RequestId);

    FreeRequest(std::move(req), send);
}

////////////////////////////////////////////////////////////////////////////////

class TServerEndpoint final
    : public IServerEndpoint
{
    // TODO
    friend class TServer;

private:
    NVerbs::IVerbsPtr Verbs;

    NVerbs::TConnectionPtr Connection;
    TString Host;
    ui32 Port;
    IServerHandlerPtr Handler;

public:
    TServerEndpoint(
            NVerbs::IVerbsPtr verbs,
            NVerbs::TConnectionPtr connection,
            TString host,
            ui32 port,
            IServerHandlerPtr handler)
        : Verbs(std::move(verbs))
        , Connection(std::move(connection))
        , Host(std::move(host))
        , Port(port)
        , Handler(std::move(handler))
    {
        Connection->context = this;
    }

    static TServerEndpoint* FromEvent(rdma_cm_event* event)
    {
        Y_ABORT_UNLESS(event->listen_id && event->listen_id->context);
        return static_cast<TServerEndpoint*>(event->listen_id->context);
    }

    void SendResponse(void* context, size_t responseBytes) override
    {
        TRequestPtr req(static_cast<TRequest*>(context));
        req->Status = RDMA_PROTO_OK;
        req->ResponseBytes = responseBytes;

        if (auto session = req->Session.lock()) {
            session->EnqueueRequest(std::move(req));
        }
    }

    void SendError(void* context, ui32 error, TStringBuf message) override
    {
        TRequestPtr req(static_cast<TRequest*>(context));
        req->Status = RDMA_PROTO_FAIL;
        req->ResponseBytes = SerializeError(
            error,
            message,
            static_cast<TStringBuf>(req->OutBuffer));

        if (auto session = req->Session.lock()) {
            session->EnqueueRequest(std::move(req));
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

struct IConnectionEventHandler
{
    virtual ~IConnectionEventHandler() = default;

    virtual void HandleConnectionEvent(rdma_cm_event* event) = 0;
};

////////////////////////////////////////////////////////////////////////////////

class TConnectionPoller final
    : public IStartable
    , private ISimpleThread
{
private:
    NVerbs::IVerbsPtr Verbs;

    IConnectionEventHandler* EventHandler;
    TLog Log;

    NVerbs::TEventChannelPtr EventChannel = NVerbs::NullPtr;
    TPollHandle PollHandle;

    TAtomic StopFlag = 0;
    TEventHandle StopEvent;

public:
    TConnectionPoller(
            NVerbs::IVerbsPtr verbs,
            IConnectionEventHandler* eventHandler,
            TLog log)
        : Verbs(std::move(verbs))
        , EventHandler(eventHandler)
        , Log(log)
    {
        EventChannel = Verbs->CreateEventChannel();
        SetNonBlock(EventChannel->fd, true);

        PollHandle.Attach(StopEvent.Handle(), EPOLLIN);
        PollHandle.Attach(EventChannel->fd, EPOLLIN, EventChannel.get());
    }

    void Start() override
    {
        ISimpleThread::Start();
    }

    void Stop() override
    {
        AtomicSet(StopFlag, 1);
        StopEvent.Set();

        Join();
    }

    NVerbs::TConnectionPtr CreateConnection()
    {
        return Verbs->CreateConnection(
            EventChannel.get(),
            nullptr,    // context
            RDMA_PS_TCP);
    }

private:
    bool ShouldStop() const
    {
        return AtomicGet(StopFlag) != 0;
    }

    void* ThreadProc() override
    {
        NCloud::SetCurrentThreadName("RDMA.CM");

        while (!ShouldStop()) {
            size_t signaled = PollHandle.Wait(POLL_TIMEOUT);

            for (size_t i = 0; i < signaled; ++i) {
                const auto& event = PollHandle.GetEvent(i);

                if (event.events && event.data.ptr) {
                    Y_ABORT_UNLESS(event.data.ptr == EventChannel.get());
                    HandleConnectionEvents();
                }
            }
        }

        return nullptr;
    }

    NVerbs::TConnectionEventPtr GetConnectionEvent()
    {
        try {
            return Verbs->GetConnectionEvent(EventChannel.get());

        } catch (const TServiceError &e) {
            STORAGE_ERROR(e.what());
            return NVerbs::NullPtr;
        }
    }

    void HandleConnectionEvents()
    {
        while (auto event = GetConnectionEvent()) {
            EventHandler->HandleConnectionEvent(event.get());
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

class TCompletionPoller final
    : public IStartable
    , private ISimpleThread
{
private:
    enum EPollEvent
    {
        Completion = 0,
        Request = 1,
    };

private:
    NVerbs::IVerbsPtr Verbs;

    TServerConfigPtr Config;
    TLog Log;

    TRCUList<TServerSessionPtr> Sessions;
    TPollHandle PollHandle;

    TAtomic StopFlag = 0;
    TEventHandle StopEvent;

public:
    TCompletionPoller(
            NVerbs::IVerbsPtr verbs,
            TServerConfigPtr config,
            TLog log)
        : Verbs(std::move(verbs))
        , Config(std::move(config))
        , Log(log)
    {
        if (Config->WaitMode == EWaitMode::Poll) {
            PollHandle.Attach(StopEvent.Handle(), EPOLLIN);
        }
    }

    void Start() override
    {
        ISimpleThread::Start();
    }

    void Stop() override
    {
        AtomicSet(StopFlag, 1);

        if (Config->WaitMode == EWaitMode::Poll) {
            StopEvent.Set();
        }

        Join();
    }

    // save session so it won't get destroyed
    void Acquire(TServerSessionPtr session)
    {
        Sessions.Add(std::move(session));
    }

    // stop polling and forget about this session
    void Release(TServerSession* session)
    {
        if (Config->WaitMode == EWaitMode::Poll) {
            PollHandle.Detach(session->CompletionChannel->fd);
        }

        Sessions.Delete([=](auto other) {
            return session == other.get();
        });
    }

    // start polling session events
    void Attach(TServerSession* session)
    {
        if (Config->WaitMode == EWaitMode::Poll) {
            PollHandle.Attach(
                session->CompletionChannel->fd,
                EPOLLIN,
                PtrEventTag(session, EPollEvent::Completion));

            PollHandle.Attach(
                session->RequestEvent.Handle(),
                EPOLLIN,
                PtrEventTag(session, EPollEvent::Request));

            Verbs->RequestCompletionEvent(session->CompletionQueue.get(), 0);
        }
    }

private:
    bool ShouldStop() const
    {
        return AtomicGet(StopFlag) != 0;
    }

    void* ThreadProc() override
    {
        SetHighestThreadPriority();
        NCloud::SetCurrentThreadName("RDMA.CQ");

        switch (Config->WaitMode) {
            case EWaitMode::Poll:
                Execute<EWaitMode::Poll>();
                break;

            case EWaitMode::BusyWait:
                Execute<EWaitMode::BusyWait>();
                break;

            case EWaitMode::AdaptiveWait:
                Execute<EWaitMode::AdaptiveWait>();
                break;
        }

        return nullptr;
    }

    void HandlePollEvent(const epoll_event& event)
    {
        auto* session = PtrFromTag<TServerSession>(event.data.ptr);

        try {
            switch (EventFromTag(event.data.ptr)) {
                case EPollEvent::Completion:
                    session->HandleCompletionEvents();
                    break;

                case EPollEvent::Request:
                    session->HandleInputRequests();
                    break;
            }
        } catch (const TServiceError& e) {
            STORAGE_ERROR(e.what());
        }
    }

    void HandlePollEvents()
    {
        // wait for completion events
        size_t signaled = PollHandle.Wait(POLL_TIMEOUT);

        for (size_t i = 0; i < signaled; ++i) {
            const auto& event = PollHandle.GetEvent(i);

            if (event.events && event.data.ptr) {
                HandlePollEvent(event);
            }
        }
    }

    bool HandleEvents()
    {
        bool hasWork = false;
        auto sessions= Sessions.Get();

        for (const auto& session: *sessions) {
            try {
                hasWork |= session->HandleInputRequests();
                hasWork |= session->HandleCompletionEvents();

            } catch (const TServiceError& e) {
                STORAGE_ERROR(e.what());
            }
        }

        return hasWork;
    }

    template <EWaitMode WaitMode>
    void Execute()
    {
        TAdaptiveWait aw(
            Config->AdaptiveWaitSleepDuration,
            Config->AdaptiveWaitSleepDelay);

        while (!ShouldStop()) {
            switch (WaitMode) {
                case EWaitMode::Poll:
                    HandlePollEvents();
                    break;

                case EWaitMode::BusyWait:
                    HandleEvents();
                    break;

                case EWaitMode::AdaptiveWait:
                    if (HandleEvents()) {
                        aw.Reset();
                    } else {
                        aw.Sleep();
                    }
            }
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

class TServer final
    : public IServer
    , public IConnectionEventHandler
{
private:
    NVerbs::IVerbsPtr Verbs;

    const ILoggingServicePtr Logging;
    const IMonitoringServicePtr Monitoring;

    TServerConfigPtr Config;
    TEndpointCountersPtr Counters;
    TLog Log;

    TVector<TServerEndpointPtr> Endpoints;
    TMutex EndpointsLock;

    TConnectionPollerPtr ConnectionPoller;
    TVector<TCompletionPollerPtr> CompletionPollers;

public:
    TServer(
        NVerbs::IVerbsPtr verbs,
        ILoggingServicePtr logging,
        IMonitoringServicePtr monitoring,
        TServerConfigPtr config);

    // called from external thread
    void Start() override;
    void Stop() override;

    IServerEndpointPtr StartEndpoint(
        TString host,
        ui32 port,
        IServerHandlerPtr handler) override;

private:
    void Listen(TServerEndpoint* endpoint);

    // called from CM thread
    void HandleConnectionEvent(rdma_cm_event* event) noexcept override;
    void HandleConnectRequest(
        TServerEndpoint* endpoint, rdma_cm_event* event) noexcept;
    void Accept(TServerEndpoint* endpoint, rdma_cm_event* event) noexcept;

    void HandleConnected(TServerSession* session) noexcept;
    void HandleDisconnected(TServerSession* session) noexcept;

    void Reject(rdma_cm_id* id, int status) noexcept;

    TCompletionPoller* PickPoller() noexcept;
};

////////////////////////////////////////////////////////////////////////////////

TServer::TServer(
        NVerbs::IVerbsPtr verbs,
        ILoggingServicePtr logging,
        IMonitoringServicePtr monitoring,
        TServerConfigPtr config)
    : Verbs(std::move(verbs))
    , Logging(std::move(logging))
    , Monitoring(std::move(monitoring))
    , Config(std::move(config))
    , Counters(new TEndpointCounters())
{
    // check basic functionality for early problem detection
    Verbs->GetDeviceList();
    Verbs->GetAddressInfo("localhost", 10020, nullptr);
}

void TServer::Start()
{
    Log = Logging->CreateLog("BLOCKSTORE_RDMA");

    STORAGE_DEBUG("start server");

    auto counters = Monitoring->GetCounters();
    auto rootGroup = counters->GetSubgroup("counters", "blockstore");
    Counters->Register(*rootGroup->GetSubgroup("component", "rdma_server"));

    CompletionPollers.resize(Config->PollerThreads);
    for (size_t i = 0; i < CompletionPollers.size(); ++i) {
        CompletionPollers[i] = std::make_unique<TCompletionPoller>(
            Verbs,
            Config,
            Log);
        CompletionPollers[i]->Start();
    }

    try {
        ConnectionPoller = std::make_unique<TConnectionPoller>(Verbs, this, Log);
        ConnectionPoller->Start();

    } catch (const TServiceError &e) {
        STORAGE_ERROR("unable to start server. " << e.what());
        Stop();
    }
}

void TServer::Stop()
{
    STORAGE_DEBUG("stop server");

    if (ConnectionPoller) {
        ConnectionPoller->Stop();
        ConnectionPoller.reset();
    }

    for (auto& poller: CompletionPollers) {
        poller->Stop();
    }

    CompletionPollers.clear();
}

IServerEndpointPtr TServer::StartEndpoint(
    TString host,
    ui32 port,
    IServerHandlerPtr handler)
{
    if (ConnectionPoller == nullptr) {
        STORAGE_ERROR("unable to create rdma endpoint. "
            << "connection poller is down");
        return nullptr;
    }

    try {
        auto endpoint = std::make_shared<TServerEndpoint>(
            Verbs,
            ConnectionPoller->CreateConnection(),
            std::move(host),
            port,
            std::move(handler));

        Listen(endpoint.get());

        with_lock (EndpointsLock) {
            Endpoints.emplace_back(endpoint);
        }

        return endpoint;

    } catch (const TServiceError& e) {
        STORAGE_ERROR("unable to create rdma endpoint. " << e.what());
        return nullptr;
    }
}

void TServer::Listen(TServerEndpoint* endpoint)
{
    rdma_addrinfo hints = {
        .ai_flags = RAI_PASSIVE,
        .ai_port_space = RDMA_PS_TCP,
    };

    auto addrinfo = Verbs->GetAddressInfo(
        endpoint->Host,
        endpoint->Port,
        &hints);

    STORAGE_INFO("LISTEN address "
        << NVerbs::PrintAddress(addrinfo->ai_src_addr));

    Verbs->BindAddress(endpoint->Connection.get(), addrinfo->ai_src_addr);
    Verbs->Listen(endpoint->Connection.get(), Config->Backlog);
}

////////////////////////////////////////////////////////////////////////////////

void TServer::HandleConnectionEvent(rdma_cm_event* event) noexcept
{
    STORAGE_DEBUG(NVerbs::GetEventName(event->event) << " received");

    switch (event->event) {
        case RDMA_CM_EVENT_ADDR_RESOLVED:
        case RDMA_CM_EVENT_ADDR_ERROR:
        case RDMA_CM_EVENT_ROUTE_RESOLVED:
        case RDMA_CM_EVENT_ROUTE_ERROR:
        case RDMA_CM_EVENT_CONNECT_RESPONSE:
        case RDMA_CM_EVENT_UNREACHABLE:
        case RDMA_CM_EVENT_REJECTED:
            // not relevant for the server
            break;

        case RDMA_CM_EVENT_MULTICAST_JOIN:
        case RDMA_CM_EVENT_MULTICAST_ERROR:
            // multicast is not used
            break;

        case RDMA_CM_EVENT_TIMEWAIT_EXIT:
            // QPs is not re-used
            break;

        case RDMA_CM_EVENT_CONNECT_REQUEST:
            HandleConnectRequest(TServerEndpoint::FromEvent(event), event);
            break;

        case RDMA_CM_EVENT_ESTABLISHED:
            HandleConnected(TServerSession::FromEvent(event));
            break;

        case RDMA_CM_EVENT_CONNECT_ERROR:
        case RDMA_CM_EVENT_DISCONNECTED:
            HandleDisconnected(TServerSession::FromEvent(event));
            break;

        case RDMA_CM_EVENT_DEVICE_REMOVAL:
        case RDMA_CM_EVENT_ADDR_CHANGE:
            // TODO
            break;
    }
}

void TServer::HandleConnectRequest(
    TServerEndpoint* endpoint,
    rdma_cm_event* event) noexcept
{
    const rdma_conn_param* connectParams = &event->param.conn;

    STORAGE_DEBUG("validate connection from "
        << NVerbs::PrintAddress(rdma_get_peer_addr(event->id))
        << " " << NVerbs::PrintConnectionParams(connectParams));

    if (connectParams->private_data == nullptr ||
        connectParams->private_data_len < sizeof(TConnectMessage) ||
        ParseMessageHeader(connectParams->private_data) < RDMA_PROTO_VERSION)
    {
        return Reject(event->id, RDMA_PROTO_INVALID_REQUEST);
    }

    const auto* connectMsg = static_cast<const TConnectMessage*>(
        connectParams->private_data);

    if (Config->StrictValidation) {
        if (connectMsg->QueueSize > Config->QueueSize ||
            connectMsg->MaxBufferSize > Config->MaxBufferSize)
        {
            return Reject(event->id, RDMA_PROTO_CONFIG_MISMATCH);
        }
    }

    Accept(endpoint, event);
}

void TServer::Accept(TServerEndpoint* endpoint, rdma_cm_event* event) noexcept
{
    try {
        auto session = std::make_shared<TServerSession>(
            Verbs,
            NVerbs::WrapPtr(event->id),
            PickPoller(),
            endpoint->Handler,
            Config,
            Counters,
            Log);

        TAcceptMessage acceptMsg = {
            .KeepAliveTimeout = SafeCast<ui16>(Config->KeepAliveTimeout.MilliSeconds()),
        };
        InitMessageHeader(&acceptMsg, RDMA_PROTO_VERSION);

        rdma_conn_param acceptParams = {
            .private_data = &acceptMsg,
            .private_data_len = sizeof(TAcceptMessage),
            .responder_resources = RDMA_MAX_RESP_RES,
            .initiator_depth = RDMA_MAX_INIT_DEPTH,
            .flow_control = 1,
            .retry_count = 7,
            .rnr_retry_count = 7,
        };

        STORAGE_DEBUG("accept connection from "
            << NVerbs::PrintAddress(rdma_get_peer_addr(event->id))
            << " " << NVerbs::PrintConnectionParams(&acceptParams));

        Verbs->Accept(event->id, &acceptParams);

        // transfer session ownership to the poller
        session->CompletionPoller->Acquire(std::move(session));

    } catch (const TServiceError& e) {
        STORAGE_ERROR(e.what())
        Reject(event->id, RDMA_PROTO_FAIL);
    }
}

void TServer::HandleConnected(TServerSession* session) noexcept
{
    try {
        session->Start();
        session->CompletionPoller->Attach(session);

    } catch (const TServiceError& e) {
        STORAGE_ERROR(e.what());
        session->Stop();
    }
}

void TServer::HandleDisconnected(TServerSession* session) noexcept
{
    session->CompletionPoller->Release(session);
}

void TServer::Reject(rdma_cm_id* id, int status) noexcept
{
    STORAGE_DEBUG("reject with status " << status);

    TRejectMessage rejectMsg = {
        .Status = SafeCast<ui16>(status),
        .QueueSize = SafeCast<ui16>(Config->QueueSize),
        .MaxBufferSize = SafeCast<ui32>(Config->MaxBufferSize),
    };
    InitMessageHeader(&rejectMsg, RDMA_PROTO_VERSION);

    try {
        Verbs->Reject(id, &rejectMsg, sizeof(TRejectMessage));

    } catch (const TServiceError& e) {
        STORAGE_ERROR(e.what());
    }
}

TCompletionPoller* TServer::PickPoller() noexcept
{
    size_t index = RandomNumber(CompletionPollers.size());
    return CompletionPollers[index].get();
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IServerPtr CreateServer(
    NVerbs::IVerbsPtr verbs,
    ILoggingServicePtr logging,
    IMonitoringServicePtr monitoring,
    TServerConfigPtr config)
{
    return std::make_shared<TServer>(
        std::move(verbs),
        std::move(logging),
        std::move(monitoring),
        std::move(config));
}

}   // namespace NCloud::NBlockStore::NRdma
