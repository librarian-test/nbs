#include "helpers.h"
#include "storage.h"
#include "appdata.h"
#include "runtime.h"
#include <ydb/core/base/appdata.h>
#include <ydb/core/base/hive.h>
#include <ydb/core/quoter/public/quoter.h>
#include <ydb/core/base/statestorage.h>
#include <ydb/core/base/statestorage_impl.h>
#include <ydb/core/base/tablet_pipe.h>
#include <ydb/core/base/tablet_resolver.h>
#include <ydb/core/node_whiteboard/node_whiteboard.h>
#include <ydb/core/blobstorage/pdisk/blobstorage_pdisk_tools.h>
#include <ydb/core/quoter/quoter_service.h>
#include <ydb/core/tablet/tablet_monitoring_proxy.h>
#include <ydb/core/tablet/resource_broker.h>
#include <ydb/core/tablet/node_tablet_monitor.h>
#include <ydb/core/tablet/tablet_list_renderer.h>
#include <ydb/core/tablet_flat/shared_sausagecache.h>
#include <ydb/core/tx/scheme_board/replica.h>
#include <ydb/core/client/server/grpc_proxy_status.h>
#include <ydb/core/scheme/tablet_scheme.h>
#include <ydb/core/util/console.h>
#include <ydb/core/base/tablet_pipecache.h>
#include <ydb/core/tx/scheme_cache/scheme_cache.h>
#include <ydb/core/tx/tx.h>
#include <ydb/core/tx/schemeshard/schemeshard.h>
#include <ydb/core/tx/scheme_board/cache.h>
#include <ydb/core/tx/columnshard/blob_cache.h>
#include <ydb/core/sys_view/service/sysview_service.h>
#include <ydb/core/statistics/stat_service.h>

#include <util/system/env.h>

static constexpr TDuration DISK_DISPATCH_TIMEOUT = NSan::PlainOrUnderSanitizer(TDuration::Seconds(10), TDuration::Seconds(20));

namespace NKikimr {

namespace NPDisk {
    extern const ui64 YdbDefaultPDiskSequence = 0x7e5700007e570000;
}

    void SetupBSNodeWarden(TTestActorRuntime& runtime, ui32 nodeIndex, TIntrusivePtr<TNodeWardenConfig> nodeWardenConfig)
    {
        runtime.AddLocalService(MakeBlobStorageNodeWardenID(runtime.GetNodeId(nodeIndex)),
            TActorSetupCmd(CreateBSNodeWarden(nodeWardenConfig), TMailboxType::Revolving, 0),
            nodeIndex);
    }

    void SetupSchemeCache(TTestActorRuntime& runtime, ui32 nodeIndex, const TString& root)
    {
        auto cacheConfig = MakeIntrusive<NSchemeCache::TSchemeCacheConfig>();
        cacheConfig->Roots.emplace_back(1, TTestTxConfig::SchemeShard, root);
        cacheConfig->Counters = new ::NMonitoring::TDynamicCounters();

        runtime.AddLocalService(MakeSchemeCacheID(),
            TActorSetupCmd(CreateSchemeBoardSchemeCache(cacheConfig.Get()), TMailboxType::Revolving, 0), nodeIndex);
    }

    void SetupTabletResolver(TTestActorRuntime& runtime, ui32 nodeIndex)
    {
        TIntrusivePtr<TTabletResolverConfig> tabletResolverConfig(new TTabletResolverConfig());
        //tabletResolverConfig->TabletCacheLimit = 1;

        IActor* tabletResolver = CreateTabletResolver(tabletResolverConfig);
        runtime.AddLocalService(MakeTabletResolverID(),
            TActorSetupCmd(tabletResolver, TMailboxType::Revolving, 0), nodeIndex);
    }

    void SetupTabletPipePeNodeCaches(TTestActorRuntime& runtime, ui32 nodeIndex, bool forceFollowers)
    {
        TIntrusivePtr<TPipePeNodeCacheConfig> leaderPipeConfig = new TPipePeNodeCacheConfig();
        leaderPipeConfig->PipeRefreshTime = TDuration::Zero();
        leaderPipeConfig->PipeConfig.RetryPolicy = {.RetryLimitCount = 3};

        TIntrusivePtr<TPipePeNodeCacheConfig> followerPipeConfig = new TPipePeNodeCacheConfig();
        followerPipeConfig->PipeRefreshTime = TDuration::Seconds(30);
        followerPipeConfig->PipeConfig.AllowFollower = true;
        followerPipeConfig->PipeConfig.RetryPolicy = {.RetryLimitCount = 3};
        followerPipeConfig->PipeConfig.ForceFollower = forceFollowers;

        runtime.AddLocalService(MakePipePeNodeCacheID(false),
            TActorSetupCmd(CreatePipePeNodeCache(leaderPipeConfig), TMailboxType::Revolving, 0), nodeIndex);
        runtime.AddLocalService(MakePipePeNodeCacheID(true),
            TActorSetupCmd(CreatePipePeNodeCache(followerPipeConfig), TMailboxType::Revolving, 0), nodeIndex);
    }

    void SetupResourceBroker(TTestActorRuntime& runtime, ui32 nodeIndex)
    {
        runtime.AddLocalService(NResourceBroker::MakeResourceBrokerID(),
            TActorSetupCmd(
                NResourceBroker::CreateResourceBrokerActor(NResourceBroker::MakeDefaultConfig(), runtime.GetDynamicCounters(0)),
                TMailboxType::Revolving, 0),
            nodeIndex);
    }

    void SetupNodeWhiteboard(TTestActorRuntime& runtime, ui32 nodeIndex)
    {
        runtime.AddLocalService(NNodeWhiteboard::MakeNodeWhiteboardServiceId(runtime.GetNodeId(nodeIndex)),
            TActorSetupCmd(NNodeWhiteboard::CreateNodeWhiteboardService(), TMailboxType::Simple, 0), nodeIndex);
    }

    void SetupNodeTabletMonitor(TTestActorRuntime& runtime, ui32 nodeIndex)
    {
        runtime.AddLocalService(
            NNodeTabletMonitor::MakeNodeTabletMonitorID(runtime.GetNodeId(nodeIndex)),
            TActorSetupCmd(
                NNodeTabletMonitor::CreateNodeTabletMonitor(
                    new NNodeTabletMonitor::TTabletStateClassifier(),
                    new NNodeTabletMonitor::TTabletListRenderer()),
                TMailboxType::Simple, 0),
            nodeIndex);
    }

    void SetupMonitoringProxy(TTestActorRuntime& runtime, ui32 nodeIndex)
    {
        NTabletMonitoringProxy::TTabletMonitoringProxyConfig tabletMonitoringProxyConfig;
        tabletMonitoringProxyConfig.SetRetryLimitCount(1u);

        runtime.AddLocalService(NTabletMonitoringProxy::MakeTabletMonitoringProxyID(),
            TActorSetupCmd(NTabletMonitoringProxy::CreateTabletMonitoringProxy(std::move(tabletMonitoringProxyConfig)),
                           TMailboxType::Revolving, 0), nodeIndex);
    }

    void SetupGRpcProxyStatus(TTestActorRuntime& runtime, ui32 nodeIndex)
    {
        runtime.AddLocalService(MakeGRpcProxyStatusID(runtime.GetNodeId(nodeIndex)),
            TActorSetupCmd(CreateGRpcProxyStatus(), TMailboxType::Revolving, 0), nodeIndex);
    }

    void SetupSharedPageCache(TTestActorRuntime& runtime, ui32 nodeIndex, NFake::TCaches caches)
    {
        auto pageCollectionCacheConfig = MakeHolder<TSharedPageCacheConfig>();
        pageCollectionCacheConfig->CacheConfig = new TCacheCacheConfig(caches.Shared, nullptr, nullptr, nullptr);
        pageCollectionCacheConfig->TotalAsyncQueueInFlyLimit = caches.AsyncQueue;
        pageCollectionCacheConfig->TotalScanQueueInFlyLimit = caches.ScanQueue;

        runtime.AddLocalService(MakeSharedPageCacheId(0),
            TActorSetupCmd(
                CreateSharedPageCache(std::move(pageCollectionCacheConfig), runtime.GetMemObserver(nodeIndex)),
                TMailboxType::ReadAsFilled,
                0),
            nodeIndex);
    }

    void SetupBlobCache(TTestActorRuntime& runtime, ui32 nodeIndex)
    {
        runtime.AddLocalService(NBlobCache::MakeBlobCacheServiceId(),
            TActorSetupCmd(
                NBlobCache::CreateBlobCache(20<<20, runtime.GetDynamicCounters(nodeIndex)),
                TMailboxType::ReadAsFilled,
                0),
            nodeIndex);
    }

    template<size_t N>
    static TIntrusivePtr<TStateStorageInfo> GenerateStateStorageInfo(const TActorId (&replicas)[N], ui64 stateStorageGroup)
    {
        auto info = MakeIntrusive<TStateStorageInfo>();
        info->StateStorageGroup = stateStorageGroup;
        info->NToSelect = N;
        info->Rings.resize(N);
        for (size_t i = 0; i < N; ++i) {
            info->Rings[i].Replicas.push_back(replicas[i]);
        }

        return info;
    }

    static TIntrusivePtr<TStateStorageInfo> GenerateStateStorageInfo(const TVector<TActorId> &replicas, ui32 NToSelect, ui32 nrings, ui32 ringSize)
    {   
        Y_ABORT_UNLESS(replicas.size() >= nrings * ringSize);
        Y_ABORT_UNLESS(NToSelect <= nrings);

        auto info = MakeIntrusive<TStateStorageInfo>();
        info->StateStorageGroup = 0;
        info->NToSelect = NToSelect;
        info->Rings.resize(nrings);
            
        ui32 inode = 0;
        for (size_t i = 0; i < nrings; ++i) {
            for (size_t j = 0; j < ringSize; ++j) {
                info->Rings[i].Replicas.push_back(replicas[inode++]);
            }
        }

        return info;
    }

    static TActorId MakeBoardReplicaID(
        const ui32 node,
        const ui64 stateStorageGroup,
        const ui32 replicaIndex
    ) {
        char x[12] = { 's', 's', 'b' };
        x[3] = (char)stateStorageGroup;
        memcpy(x + 5, &replicaIndex, sizeof(ui32));
        return TActorId(node, TStringBuf(x, 12));
    }

    void SetupCustomStateStorage(
        TTestActorRuntime &runtime,
        ui32 NToSelect, 
        ui32 nrings, 
        ui32 ringSize,
        ui64 stateStorageGroup)
    {   
        TVector<TActorId> ssreplicas;
        for (size_t i = 0; i < nrings * ringSize; ++i) {
            ssreplicas.push_back(MakeStateStorageReplicaID(runtime.GetNodeId(i), stateStorageGroup, i));
        }

        TVector<TActorId> breplicas;
        for (size_t i = 0; i < nrings * ringSize; ++i) {
            breplicas.push_back(MakeBoardReplicaID(runtime.GetNodeId(i), stateStorageGroup, i));
        }

        TVector<TActorId> sbreplicas;
        for (size_t i = 0; i < nrings * ringSize; ++i) {
            sbreplicas.push_back(MakeSchemeBoardReplicaID(runtime.GetNodeId(i), stateStorageGroup, i));
        }

        const TActorId ssproxy = MakeStateStorageProxyID(stateStorageGroup);

        auto ssInfo = GenerateStateStorageInfo(ssreplicas, NToSelect, nrings, ringSize);
        auto sbInfo = GenerateStateStorageInfo(sbreplicas, NToSelect, nrings, ringSize);
        auto bInfo = GenerateStateStorageInfo(breplicas, NToSelect, nrings, ringSize);

        
        for (ui32 ssIndex = 0; ssIndex < nrings * ringSize; ++ssIndex) {
            runtime.AddLocalService(ssreplicas[ssIndex],
                TActorSetupCmd(CreateStateStorageReplica(ssInfo.Get(), ssIndex), TMailboxType::Revolving, 0), ssIndex);
            runtime.AddLocalService(sbreplicas[ssIndex],
                TActorSetupCmd(CreateSchemeBoardReplica(sbInfo.Get(), ssIndex), TMailboxType::Revolving, 0), ssIndex);
            runtime.AddLocalService(breplicas[ssIndex],
                TActorSetupCmd(CreateStateStorageBoardReplica(bInfo.Get(), ssIndex), TMailboxType::Revolving, 0), ssIndex);
        }

        for (ui32 nodeIndex = 0; nodeIndex < runtime.GetNodeCount(); ++nodeIndex) {
            runtime.AddLocalService(ssproxy,
                    TActorSetupCmd(CreateStateStorageProxy(ssInfo.Get(), bInfo.Get(), sbInfo.Get()), TMailboxType::Revolving, 0), nodeIndex);
        }
    }

    
    void SetupStateStorage(TTestActorRuntime& runtime, ui32 nodeIndex, ui64 stateStorageGroup, bool firstNode)
    {
        const TActorId ssreplicas[3] = {
            MakeStateStorageReplicaID(runtime.GetNodeId(0), stateStorageGroup, 0),
            MakeStateStorageReplicaID(runtime.GetNodeId(0), stateStorageGroup, 1),
            MakeStateStorageReplicaID(runtime.GetNodeId(0), stateStorageGroup, 2),
        };

        const TActorId breplicas[3] = {
            MakeBoardReplicaID(runtime.GetNodeId(0), stateStorageGroup, 0),
            MakeBoardReplicaID(runtime.GetNodeId(0), stateStorageGroup, 1),
            MakeBoardReplicaID(runtime.GetNodeId(0), stateStorageGroup, 2),
        };

        const TActorId sbreplicas[3] = {
            MakeSchemeBoardReplicaID(runtime.GetNodeId(0), stateStorageGroup, 0),
            MakeSchemeBoardReplicaID(runtime.GetNodeId(0), stateStorageGroup, 1),
            MakeSchemeBoardReplicaID(runtime.GetNodeId(0), stateStorageGroup, 2),
        };

        const TActorId ssproxy = MakeStateStorageProxyID(stateStorageGroup);

        auto ssInfo = GenerateStateStorageInfo(ssreplicas, stateStorageGroup);
        auto sbInfo = GenerateStateStorageInfo(sbreplicas, stateStorageGroup);
        auto bInfo = GenerateStateStorageInfo(breplicas, stateStorageGroup);

        if (!firstNode || nodeIndex == 0) {
            for (ui32 i = 0; i < 3; ++i) {
                runtime.AddLocalService(ssreplicas[i],
                    TActorSetupCmd(CreateStateStorageReplica(ssInfo.Get(), i), TMailboxType::Revolving, 0), nodeIndex);
                runtime.AddLocalService(sbreplicas[i],
                    TActorSetupCmd(CreateSchemeBoardReplica(sbInfo.Get(), i), TMailboxType::Revolving, 0), nodeIndex);
                runtime.AddLocalService(breplicas[i],
                    TActorSetupCmd(CreateStateStorageBoardReplica(bInfo.Get(), i), TMailboxType::Revolving, 0), nodeIndex);
            }
        }

        runtime.AddLocalService(ssproxy,
            TActorSetupCmd(CreateStateStorageProxy(ssInfo.Get(), bInfo.Get(), sbInfo.Get()), TMailboxType::Revolving, 0), nodeIndex);
    }

    static void SetupStateStorageGroups(TTestActorRuntime& runtime, ui32 nodeIndex, TAppPrepare& app)
    {
        TSet<ui32> stateStorageGroups;
        for (auto it = app.Domains->Domains.begin(); it != app.Domains->Domains.end(); ++it) {
            ui32 stateStorageGroup = it->second->DefaultStateStorageGroup;
            if (stateStorageGroups.find(stateStorageGroup) == stateStorageGroups.end()) {
                stateStorageGroups.insert(stateStorageGroup);

                SetupStateStorage(runtime, nodeIndex, stateStorageGroup, true);
            }
        }
    }

    void SetupQuoterService(TTestActorRuntime& runtime, ui32 nodeIndex)
    {
        runtime.AddLocalService(MakeQuoterServiceID(),
                TActorSetupCmd(CreateQuoterService(), TMailboxType::HTSwap, 0),
                nodeIndex);
    }

    void SetupSysViewService(TTestActorRuntime& runtime, ui32 nodeIndex)
    {
        runtime.AddLocalService(NSysView::MakeSysViewServiceID(runtime.GetNodeId(nodeIndex)),
                TActorSetupCmd(NSysView::CreateSysViewServiceForTests().Release(), TMailboxType::Revolving, 0),
                nodeIndex);
    }

    void SetupStatService(TTestActorRuntime& runtime, ui32 nodeIndex)
    {
        runtime.AddLocalService(NStat::MakeStatServiceID(runtime.GetNodeId(nodeIndex)),
                TActorSetupCmd(NStat::CreateStatService().Release(), TMailboxType::HTSwap, 0),
                nodeIndex);
    }

    void SetupBasicServices(TTestActorRuntime& runtime, TAppPrepare& app, bool mock,
                            NFake::INode* factory, NFake::TStorage storage, NFake::TCaches caches, bool forceFollowers)
    {
        runtime.SetDispatchTimeout(storage.UseDisk ? DISK_DISPATCH_TIMEOUT : DEFAULT_DISPATCH_TIMEOUT);

        TTestStorageFactory disk(runtime, storage, mock);

        {
            NKikimrBlobStorage::TNodeWardenServiceSet bsConfig;
            Y_ABORT_UNLESS(google::protobuf::TextFormat::ParseFromString(disk.MakeTextConf(*app.Domains), &bsConfig));
            app.SetBSConf(std::move(bsConfig));
        }

        if (app.Domains->Domains.empty()) {
            app.AddDomain(TDomainsInfo::TDomain::ConstructEmptyDomain("dc-1").Release());
            app.AddHive(0, 0);
        }

        for (ui32 nodeIndex = 0; nodeIndex < runtime.GetNodeCount(); ++nodeIndex) {
            SetupStateStorageGroups(runtime, nodeIndex, app);
            NKikimrProto::TKeyConfig keyConfig;
            if (const auto it = app.Keys.find(nodeIndex); it != app.Keys.end()) {
                keyConfig = it->second;
            }
            SetupBSNodeWarden(runtime, nodeIndex, disk.MakeWardenConf(*app.Domains, keyConfig));

            SetupTabletResolver(runtime, nodeIndex);
            SetupTabletPipePeNodeCaches(runtime, nodeIndex, forceFollowers);
            SetupResourceBroker(runtime, nodeIndex);
            SetupSharedPageCache(runtime, nodeIndex, caches);
            SetupBlobCache(runtime, nodeIndex);
            SetupSysViewService(runtime, nodeIndex);
            SetupQuoterService(runtime, nodeIndex);
            SetupStatService(runtime, nodeIndex);

            if (factory)
                factory->Birth(nodeIndex);
        }

        runtime.Initialize(app.Unwrap());

        for (ui32 nodeIndex = 0; nodeIndex < runtime.GetNodeCount(); ++nodeIndex) {
            // NodeWarden (and its actors) relies on timers to work correctly
            auto blobStorageActorId = runtime.GetLocalServiceId(
                MakeBlobStorageNodeWardenID(runtime.GetNodeId(nodeIndex)),
                nodeIndex);
            Y_ABORT_UNLESS(blobStorageActorId, "Missing node warden on node %" PRIu32, nodeIndex);
            runtime.EnableScheduleForActor(blobStorageActorId);

            // SysView Service uses Scheduler to send counters
            auto sysViewServiceId = runtime.GetLocalServiceId(
                NSysView::MakeSysViewServiceID(runtime.GetNodeId(nodeIndex)), nodeIndex);
            Y_ABORT_UNLESS(sysViewServiceId, "Missing SysView Service on node %" PRIu32, nodeIndex);
            runtime.EnableScheduleForActor(sysViewServiceId);
        }


        if (!mock && !runtime.IsRealThreads()) {
            ui32 evNum = disk.DomainsNum * disk.DisksInDomain;
            TDispatchOptions options;
            options.FinalEvents.push_back(
                TDispatchOptions::TFinalEventCondition(TEvBlobStorage::EvLocalRecoveryDone, evNum));
            runtime.DispatchEvents(options);
        }
    }
}
