#pragma once

#include "public.h"

#include <cloud/blockstore/libs/daemon/common/bootstrap.h>

namespace NCloud::NBlockStore::NServer {

////////////////////////////////////////////////////////////////////////////////

class TBootstrapLocal final
    : public TBootstrapBase
{
private:
    TConfigInitializerLocalPtr Configs;

public:
    TBootstrapLocal(IDeviceHandlerFactoryPtr deviceHandlerFactory);
    ~TBootstrapLocal();

    TProgramShouldContinue& GetShouldContinue() override;

protected:
    TConfigInitializerCommonPtr InitConfigs(int argc, char** argv) override;

    IStartable* GetActorSystem() override        { return nullptr; }
    IStartable* GetAsyncLogger() override        { return nullptr; }
    IStartable* GetStatsAggregator() override    { return nullptr; }
    IStartable* GetClientPercentiles() override  { return nullptr; }
    IStartable* GetStatsUploader() override      { return nullptr; }
    IStartable* GetYdbStorage() override         { return nullptr; }
    IStartable* GetTraceSerializer() override    { return nullptr; }
    IStartable* GetLogbrokerService() override   { return nullptr; }
    IStartable* GetNotifyService() override      { return nullptr; }
    IStartable* GetCgroupStatsFetcher() override { return nullptr; }
    IStartable* GetIamTokenClient() override     { return nullptr; }

    void InitKikimrService() override;
    void InitAuthService() override;
};

}   // namespace NCloud::NBlockStore::NServer
