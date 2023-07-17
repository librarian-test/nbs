#include "forward_write_and_mark_used.h"

#include <library/cpp/actors/testlib/test_runtime.h>
#include <library/cpp/testing/unittest/registar.h>

namespace NCloud::NBlockStore::NStorage {

static NActors::TActorId EdgeActor;

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TForwardWriteAndMarkUsedTests)
{
    struct TSetupEnvironment
        : public TCurrentTestCase
    {
        NActors::TTestActorRuntimeBase ActorSystem;

        void SetUp(NUnitTest::TTestContext&) override
        {
            static const TString volumeName = "VOLUME";
            ActorSystem.Initialize();
            ActorSystem.SetDispatchTimeout(TDuration::Seconds(5));
            ActorSystem.GetLogSettings(0)->Append(
                TBlockStoreComponents::VOLUME,
                TBlockStoreComponents::VOLUME + 1,
                [](int){ return volumeName; });

            EdgeActor = ActorSystem.AllocateEdgeActor();
        }
    };

    struct TSetupParallelEnvironment
        : public TSetupEnvironment
    {
        TActorId WriteActor;

        void SetUp(NUnitTest::TTestContext& ctx) override
        {
            TSetupEnvironment::SetUp(ctx);
            WriteActor = ActorSystem.Register(
                new TWriteAndMarkUsedActor<TEvService::TWriteBlocksMethod>{
                    MakeIntrusive<TRequestInfo>(
                        EdgeActor,
                        0ull,
                        MakeIntrusive<TCallContext>()),
                    {},
                    512,
                    false,
                    0,
                    EdgeActor,
                    0,
                    EdgeActor});
        }
    };

    struct TSetupConsistentlyEnvironment
        : public TSetupEnvironment
    {
        TActorId WriteActor;

        void SetUp(NUnitTest::TTestContext& ctx) override
        {
            TSetupEnvironment::SetUp(ctx);
            WriteActor = ActorSystem.Register(
                new TWriteAndMarkUsedActor<TEvService::TWriteBlocksMethod>{
                    MakeIntrusive<TRequestInfo>(
                        EdgeActor,
                        0ull,
                        MakeIntrusive<TCallContext>()),
                    {},
                    512,
                    true,
                    0,
                    EdgeActor,
                    0,
                    EdgeActor});
        }
    };

    Y_UNIT_TEST_F(ParallelShouldWriteAndMarkSuccess, TSetupParallelEnvironment)
    {
        ActorSystem.GrabEdgeEvent<TEvService::TWriteBlocksMethod::TRequest>();
        ActorSystem.GrabEdgeEvent<TEvVolume::TEvUpdateUsedBlocksRequest>();

        ActorSystem.Send(new NActors::IEventHandle(
            WriteActor,
            EdgeActor,
            new TEvService::TWriteBlocksMethod::TResponse()));

        try {
            ActorSystem.GrabEdgeEvent<TEvVolume::TEvUpdateUsedBlocksRequest>();
            UNIT_ASSERT_C(0, "Exception hasn't been thrown");
        } catch (...) {}

        ActorSystem.Send(new NActors::IEventHandle(
            WriteActor,
            EdgeActor,
            new TEvVolume::TEvUpdateUsedBlocksResponse()));

        ActorSystem.GrabEdgeEvent<TEvService::TWriteBlocksMethod::TResponse>();
        ActorSystem.GrabEdgeEvent<TEvVolumePrivate::TEvWriteOrZeroCompleted>();
    }

    Y_UNIT_TEST_F(ParallelShouldWriteAndMarkError, TSetupParallelEnvironment)
    {
        ActorSystem.GrabEdgeEvent<TEvService::TWriteBlocksMethod::TRequest>();
        ActorSystem.GrabEdgeEvent<TEvVolume::TEvUpdateUsedBlocksRequest>();

        ActorSystem.Send(new NActors::IEventHandle(
            WriteActor,
            EdgeActor,
            new TEvService::TWriteBlocksMethod::TResponse(MakeError(E_REJECTED))));

        ActorSystem.Send(new NActors::IEventHandle(
            WriteActor,
            EdgeActor,
            new TEvVolume::TEvUpdateUsedBlocksResponse()));

        UNIT_ASSERT_EQUAL(
            ActorSystem.GrabEdgeEvent<
                TEvService::TWriteBlocksMethod::TResponse>()->GetError().code(),
            E_REJECTED);

        UNIT_ASSERT_EQUAL(
            ActorSystem.GrabEdgeEvent<
                TEvVolumePrivate::TEvWriteOrZeroCompleted>()->ResultCode,
            E_REJECTED);
    }

    Y_UNIT_TEST_F(ConsistentlyShouldWriteAndMarkSuccess, TSetupConsistentlyEnvironment)
    {
        ActorSystem.GrabEdgeEvent<TEvService::TWriteBlocksMethod::TRequest>();
        try {
            ActorSystem.GrabEdgeEvent<TEvVolume::TEvUpdateUsedBlocksRequest>();
            UNIT_ASSERT_C(0, "Exception hasn't been thrown");
        } catch (...) {}

        ActorSystem.Send(new NActors::IEventHandle(
            WriteActor,
            EdgeActor,
            new TEvService::TWriteBlocksMethod::TResponse()));

        ActorSystem.GrabEdgeEvent<TEvVolume::TEvUpdateUsedBlocksRequest>();

        ActorSystem.Send(new NActors::IEventHandle(
            WriteActor,
            EdgeActor,
            new TEvVolume::TEvUpdateUsedBlocksResponse()));

        ActorSystem.GrabEdgeEvent<TEvService::TWriteBlocksMethod::TResponse>();
        ActorSystem.GrabEdgeEvent<TEvVolumePrivate::TEvWriteOrZeroCompleted>();
    }

    Y_UNIT_TEST_F(ConsistentlyShouldWriteAndMarkError, TSetupConsistentlyEnvironment)
    {
        ActorSystem.GrabEdgeEvent<TEvService::TWriteBlocksMethod::TRequest>();

        ActorSystem.Send(new NActors::IEventHandle(
            WriteActor,
            EdgeActor,
            new TEvService::TWriteBlocksMethod::TResponse(MakeError(E_REJECTED))));

        try {
            ActorSystem.GrabEdgeEvent<TEvVolume::TEvUpdateUsedBlocksRequest>();
            UNIT_ASSERT_C(0, "Exception hasn't been thrown");
        } catch (...) {}

        UNIT_ASSERT_EQUAL(
            ActorSystem.GrabEdgeEvent<
                TEvService::TWriteBlocksMethod::TResponse>()->GetError().code(),
            E_REJECTED);

        UNIT_ASSERT_EQUAL(
            ActorSystem.GrabEdgeEvent<
                TEvVolumePrivate::TEvWriteOrZeroCompleted>()->ResultCode,
            E_REJECTED);
    }
}

}   // namespace NCloud::NBlockStore::NStorage
