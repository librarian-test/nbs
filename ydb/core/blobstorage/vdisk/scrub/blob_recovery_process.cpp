#include "blob_recovery_impl.h"

namespace NKikimr {

    void TBlobRecoveryActor::AddBlobQuery(const TLogoBlobID& id, NMatrix::TVectorType needed,
            const std::shared_ptr<TInFlightContext>& context, TEvRecoverBlobResult::TItem *item) {
        STLOG(PRI_DEBUG, BS_VDISK_SCRUB, VDS32, VDISKP(LogPrefix, "AddBlobQuery"), (SelfId, SelfId()),
            (Id, id), (Needed, needed), (RequestId, context->RequestId));
        const TInstant deadline = context->Iterator->first;
        const TBlobStorageGroupType& gtype = Info->Type;
        TBlobStorageGroupInfo::TOrderNums nums;
        Info->GetTopology().PickSubgroup(id.Hash(), nums);
        ui32 blobReplyCounter = 0;
        for (ui32 i = 0; i < nums.size(); ++i) {
            const TVDiskID& vdiskId = Info->GetVDiskId(i); // obtain VDisk
            if (TVDiskIdShort(vdiskId) == VCtx->ShortSelfVDisk) {
                continue;
            }
            switch (TIngress::IngressMode(gtype)) {
                case TIngress::EMode::GENERIC:
                    ui32 maxSize;
                    maxSize = 0;
                    if (gtype.GetErasure() == TBlobStorageGroupType::ErasureMirror3dc) {
                        maxSize += gtype.PartSize(TLogoBlobID(id, i % 3 + 1));
                    } else {
                        for (ui32 k = 0; k < gtype.TotalPartCount(); ++k) {
                            maxSize += i >= gtype.TotalPartCount() || k == i ? gtype.PartSize(TLogoBlobID(id, k + 1)) : 0;
                        }
                    }
                    AddExtremeQuery(vdiskId, id, deadline, maxSize);
                    break;

                case TIngress::EMode::MIRROR3OF4:
                    AddExtremeQuery(vdiskId, id, deadline, gtype.PartSize(TLogoBlobID(id, 1)) +
                        gtype.PartSize(TLogoBlobID(id, 2)));
                    break;
            }
            ++blobReplyCounter;
        }
        VGetResultMap.emplace(id, TPerBlobInfo{context->Iterator->first, context, item, blobReplyCounter});
    }

    void TBlobRecoveryActor::AddExtremeQuery(const TVDiskID& vdiskId, const TLogoBlobID& id, TInstant deadline, ui32 worstReplySize) {
        STLOG(PRI_DEBUG, BS_VDISK_SCRUB, VDS33, VDISKP(LogPrefix, "AddExtremeQuery"), (SelfId, SelfId()),
            (VDiskId, vdiskId), (Id, id), (WorstReplySize, worstReplySize));

        TQuery& query = Queries[vdiskId];

        const ui32 maxReplySize = 10000000; // FIXME
        if (query.VGet && query.WorstReplySize + worstReplySize > maxReplySize) { // send the request on overflow
            query.Pending.push_back(std::move(query.VGet));
            query.WorstReplySize = 0;
        }

        if (!query.VGet) {
            query.VGet = TEvBlobStorage::TEvVGet::CreateExtremeDataQuery(vdiskId, deadline,
                NKikimrBlobStorage::EGetHandleClass::AsyncRead);
        }

        query.VGet->AddExtremeQuery(id, 0, 0);
        query.WorstReplySize += worstReplySize;
    }

    void TBlobRecoveryActor::SendPendingQueries() {
        for (auto& [vdiskId, query] : std::exchange(Queries, {})) {
            Y_ABORT_UNLESS(query.VGet);
            query.Pending.push_back(std::move(query.VGet));
            auto queueIt = Queues.find(vdiskId);
            Y_ABORT_UNLESS(queueIt != Queues.end());
            for (auto& vget : query.Pending) {
                STLOG(PRI_DEBUG, BS_VDISK_SCRUB, VDS34, VDISKP(LogPrefix, "sending TEvVGet"), (SelfId, SelfId()),
                    (Msg, vget->ToString()));
                Send(queueIt->second.QueueActorId, vget.release());
            }
        }
    }

    void TBlobRecoveryActor::Handle(TEvBlobStorage::TEvVGetResult::TPtr ev) {
        STLOG(PRI_DEBUG, BS_VDISK_SCRUB, VDS35, VDISKP(LogPrefix, "received TEvVGetResult"), (SelfId, SelfId()),
            (Msg, ev->Get()->ToString()));

        const auto& record = ev->Get()->Record;
        for (const auto& res : record.GetResult()) {
            const TLogoBlobID& id = LogoBlobIDFromLogoBlobID(res.GetBlobID());
            const TLogoBlobID& fullId = id.FullID(); // whole blob id
            auto r = VGetResultMap.equal_range(fullId);
            for (auto it = r.first; it != r.second; ) {
                TPerBlobInfo& info = it->second;
                if (auto context = info.Context.lock()) { // context acquired, request is still intact
                    auto& item = *info.Item; // only here we can access item, after obtaining context pointer
                    TRope data = ev->Get()->GetBlobData(res);
                    bool update = false;
                    if (res.GetStatus() == NKikimrProto::OK && data) {
                        item.SetPartData(id, std::move(data));
                        update = true;
                    }
                    const bool term = !--info.BlobReplyCounter;
                    if (item.Status == NKikimrProto::UNKNOWN && (term || update)) {
                        const NKikimrProto::EReplyStatus prevStatus = std::exchange(item.Status, ProcessItemData(item));
                        if (item.Status == NKikimrProto::UNKNOWN && term) { // not enough parts to fulfill request
                            item.Status = NKikimrProto::NODATA;
                        }
                        STLOG(PRI_DEBUG, BS_VDISK_SCRUB, VDS36, VDISKP(LogPrefix, "processing item"),
                            (SelfId, SelfId()), (RequestId, context->RequestId), (Id, id),
                            (Status, res.GetStatus()), (Last, term), (DataUpdated, update),
                            (EntryStatus, prevStatus), (ExitStatus, item.Status));
                    }
                    if (item.Status != NKikimrProto::UNKNOWN && !--context->NumUnrespondedBlobs) {
                        context->SendResult(SelfId());
                        InFlight.erase(context->Iterator);
                    }
                    ++it;
                } else { // request deadlined or canceled, we erase it from the map
                    it = VGetResultMap.erase(it);
                }
            }
        }
    }

    NKikimrProto::EReplyStatus TBlobRecoveryActor::ProcessItemData(TEvRecoverBlobResult::TItem& item) {
        if (item.GetAvailableParts().IsSupersetOf(item.Needed)) {
            return NKikimrProto::OK;
        }
        const ui32 numParts = PopCount(item.PartsMask);
        if (numParts >= Info->Type.MinimalRestorablePartCount()) {
            Y_VERIFY_DEBUG(item.Parts.size() == Info->Type.TotalPartCount());

            ui32 restoreMask = 0;
            for (ui8 i = item.Needed.FirstPosition(); i != item.Needed.GetSize(); i = item.Needed.NextPosition(i)) {
                restoreMask |= 1 << i;
            }
            restoreMask &= ~item.PartsMask;

            ErasureRestore((TErasureType::ECrcMode)item.BlobId.CrcMode(), Info->Type, item.BlobId.BlobSize(), nullptr,
                item.Parts, restoreMask);
            item.PartsMask |= restoreMask;

            // clear metadata parts in mirror erasures
            for (ui32 i = 0; i < item.Parts.size(); ++i) {
                if (!Info->Type.PartSize(TLogoBlobID(item.BlobId, i + 1))) {
                    item.Parts[i] = TRope();
                }
            }
            return NKikimrProto::OK;
        } else {
            return NKikimrProto::UNKNOWN;
        }
    }

} // NKikimr
