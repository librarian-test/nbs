#pragma once

#include "defs.h"

#include <ydb/core/protos/subdomains.pb.h>
#include <ydb/core/protos/tx_mediator_timecast.pb.h>

#include <util/stream/str.h>
#include <util/string/builder.h>

namespace NKikimr {

class TMediatorTimecastEntry : public TThrRefBase {
    TAtomic Step;
public:
    TMediatorTimecastEntry()
        : Step(0)
    {}

    ui64 Get(ui64 tabletId) const;
    void Update(ui64 step, ui64 *exemption, ui64 exsz);
};

class TMediatorTimecastReadStep : public TThrRefBase {
public:
    using TPtr = TIntrusivePtr<TMediatorTimecastReadStep>;
    using TCPtr = TIntrusiveConstPtr<TMediatorTimecastReadStep>;

    TMediatorTimecastReadStep(ui64 nextReadStep = 0)
        : NextReadStep{ nextReadStep }
    { }

    ui64 Get() const {
        return NextReadStep.load();
    }

    void Update(ui64 nextReadStep) {
        NextReadStep.store(nextReadStep);
    }

private:
    std::atomic<ui64> NextReadStep;
};

struct TEvMediatorTimecast {
    enum EEv {
        // local part
        EvRegisterTablet = EventSpaceBegin(TKikimrEvents::ES_TX_MEDIATORTIMECAST),
        EvUnregisterTablet,
        EvWaitPlanStep,
        EvSubscribeReadStep,
        EvUnsubscribeReadStep,
        EvWaitReadStep,

        EvRegisterTabletResult = EvRegisterTablet + 1 * 512,
        EvNotifyPlanStep,
        EvSubscribeReadStepResult,
        EvNotifyReadStep,

        // mediator part
        EvWatch = EvRegisterTablet + 2 * 512,

        EvUpdate = EvRegisterTablet + 3 * 512,

        EvEnd
    };

    static_assert(EvEnd < EventSpaceEnd(TKikimrEvents::ES_TX_MEDIATORTIMECAST), "expected EvEnd < EventSpaceEnd()");

    struct TEvRegisterTablet : public TEventLocal<TEvRegisterTablet, EvRegisterTablet> {
        const ui64 TabletId;
        NKikimrSubDomains::TProcessingParams ProcessingParams;


        TEvRegisterTablet(ui64 tabletId, const NKikimrSubDomains::TProcessingParams &processing)
            : TabletId(tabletId)
            , ProcessingParams(processing)
        {}

        TString ToString() const {
            TStringStream str;
            str << "{TEvRegisterTablet";
            str << " TabletId# " << TabletId;
            if (ProcessingParams.HasVersion()) {
                str << " ProcessingParams { " <<  ProcessingParams.ShortDebugString() << " }";
            }
            str << "}";
            return str.Str();
        }
    };

    struct TEvRegisterTabletResult : public TEventLocal<TEvRegisterTabletResult, EvRegisterTabletResult> {
        const ui64 TabletId;
        const TIntrusivePtr<TMediatorTimecastEntry> Entry;

        TEvRegisterTabletResult(ui64 tabletId, TIntrusivePtr<TMediatorTimecastEntry> &entry)
            : TabletId(tabletId)
            , Entry(entry)
        {}

        TString ToString() const {
            TStringStream str;
            str << "{TEvRegisterTabletResult";
            str << " TabletId# " << TabletId;
            if (Entry) {
                str << " Entry# " << Entry->Get(TabletId);
            } else {
                str << " Entry# nullptr";
            }
            str << "}";
            return str.Str();
        }
    };

    struct TEvUnregisterTablet : public TEventLocal<TEvUnregisterTablet, EvUnregisterTablet> {
        const ui64 TabletId;

        TEvUnregisterTablet(ui64 tabletId)
            : TabletId(tabletId)
        {}

        TString ToString() const {
            TStringStream str;
            str << "{TEvUnregisterTablet";
            str << " TabletId# " << TabletId;
            str << "}";
            return str.Str();
        }
    };

    struct TEvWaitPlanStep : public TEventLocal<TEvWaitPlanStep, EvWaitPlanStep> {
        const ui64 TabletId;
        const ui64 PlanStep;

        TEvWaitPlanStep(ui64 tabletId, ui64 planStep)
            : TabletId(tabletId)
            , PlanStep(planStep)
        { }

        TString ToString() const {
            return TStringBuilder()
                << "{TEvWaitPlanStep"
                << " TabletId# " << TabletId
                << " PlanStep# " << PlanStep
                << "}";
        }
    };

    struct TEvNotifyPlanStep : public TEventLocal<TEvNotifyPlanStep, EvNotifyPlanStep> {
        const ui64 TabletId;
        const ui64 PlanStep;

        TEvNotifyPlanStep(ui64 tabletId, ui64 planStep)
            : TabletId(tabletId)
            , PlanStep(planStep)
        { }

        TString ToString() const {
            return TStringBuilder()
                << "{TEvNotifyPlanStep"
                << " TabletId# " << TabletId
                << " PlanStep# " << PlanStep
                << "}";
        }
    };

    struct TEvSubscribeReadStep : public TEventLocal<TEvSubscribeReadStep, EvSubscribeReadStep> {
        const ui64 CoordinatorId;

        explicit TEvSubscribeReadStep(ui64 coordinatorId)
            : CoordinatorId(coordinatorId)
        {
            Y_ABORT_UNLESS(coordinatorId != 0);
        }

        TString ToString() const {
            return TStringBuilder()
                << ToStringHeader() << "{"
                << " CoordinatorId# " << CoordinatorId
                << " }";
        }
    };

    struct TEvUnsubscribeReadStep : public TEventLocal<TEvUnsubscribeReadStep, EvUnsubscribeReadStep> {
        const ui64 CoordinatorId;

        explicit TEvUnsubscribeReadStep(ui64 coordinatorId = 0)
            : CoordinatorId(coordinatorId)
        { }

        TString ToString() const {
            return TStringBuilder()
                << ToStringHeader() << "{"
                << " CoordinatorId# " << CoordinatorId
                << " }";
        }
    };

    struct TEvSubscribeReadStepResult : public TEventLocal<TEvSubscribeReadStepResult, EvSubscribeReadStepResult> {
        const ui64 CoordinatorId;
        const ui64 LastReadStep;
        const ui64 NextReadStep;
        const TMediatorTimecastReadStep::TCPtr ReadStep;

        TEvSubscribeReadStepResult(
                ui64 coordinatorId,
                ui64 lastReadStep,
                ui64 nextReadStep,
                TMediatorTimecastReadStep::TCPtr readStep)
            : CoordinatorId(coordinatorId)
            , LastReadStep(lastReadStep)
            , NextReadStep(nextReadStep)
            , ReadStep(std::move(readStep))
        {
            Y_ABORT_UNLESS(ReadStep);
        }

        TString ToString() const {
            return TStringBuilder()
                << ToStringHeader() << "{"
                << " CoordinatorId# " << CoordinatorId
                << " LastReadStep# " << LastReadStep
                << " NextReadStep# " << NextReadStep
                << " ReadStep# " << ReadStep->Get()
                << " }";
        }
    };

    struct TEvWaitReadStep : public TEventLocal<TEvWaitReadStep, EvWaitReadStep> {
        const ui64 CoordinatorId;
        const ui64 ReadStep;

        TEvWaitReadStep(ui64 coordinatorId, ui64 readStep)
            : CoordinatorId(coordinatorId)
            , ReadStep(readStep)
        { }

        TString ToString() const {
            return TStringBuilder()
                << ToStringHeader() << "{"
                << " CoordinatorId# " << CoordinatorId
                << " ReadStep# " << ReadStep
                << " }";
        }
    };

    struct TEvNotifyReadStep : public TEventLocal<TEvNotifyReadStep, EvNotifyReadStep> {
        const ui64 CoordinatorId;
        const ui64 ReadStep;

        TEvNotifyReadStep(ui64 coordinatorId, ui64 readStep)
            : CoordinatorId(coordinatorId)
            , ReadStep(readStep)
        { }

        TString ToString() const {
            return TStringBuilder()
                << ToStringHeader() << "{"
                << " CoordinatorId# " << CoordinatorId
                << " ReadStep# " << ReadStep
                << " }";
        }
    };

    struct TEvWatch : public TEventPB<TEvWatch, NKikimrTxMediatorTimecast::TEvWatch, EvWatch> {
        TEvWatch()
        {}

        TEvWatch(ui32 bucket)
        {
            Record.AddBucket(bucket);
        }

        TString ToString() const {
            TStringStream str;
            str << "{TEvWatch";
            for (size_t i = 0; i < Record.BucketSize(); ++i) {
                str << " Bucket# " << Record.GetBucket(i);
            }
            str << "}";
            return str.Str();
        }
    };

    struct TEvUpdate : public TEventPB<TEvUpdate, NKikimrTxMediatorTimecast::TEvUpdate, EvUpdate> {
        TString ToString() const {
            TStringStream str;
            str << "{TEvUpdate ";
            if (Record.HasMediator()) {
                str << " Mediator# " << Record.GetMediator();
            }
            if (Record.HasBucket()) {
                str << " Bucket# " << Record.GetBucket();
            }
            if (Record.HasTimeBarrier()) {
                str << " TimeBarrier# " << Record.GetTimeBarrier();
            }
            for (size_t i = 0; i < Record.ExemptionSize(); ++i) {
                str << " Exemption# " << Record.GetExemption(i);
            }
            str << "}";
            return str.Str();
        }
    };
};

IActor* CreateMediatorTimecastProxy();

inline TActorId MakeMediatorTimecastProxyID() {
    return TActorId(0, TStringBuf("txmdtimecast"));
}

}
