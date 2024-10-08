#pragma once

#include <contrib/ydb/core/protos/import.pb.h>
#include <contrib/ydb/public/api/protos/ydb_operation.pb.h>
#include <contrib/ydb/public/lib/operation_id/operation_id.h>

#include <util/string/cast.h>

namespace NKikimr {
namespace NGRpcService {

struct TImportConv {
    static Ydb::TOperationId MakeOperationId(const ui64 id, NKikimrImport::TImport::SettingsCase kind) {
        Ydb::TOperationId operationId;
        operationId.SetKind(Ydb::TOperationId::IMPORT);
        NOperationId::AddOptionalValue(operationId, "id", ToString(id));

        switch (kind) {
        case NKikimrImport::TImport::kImportFromS3Settings:
            NOperationId::AddOptionalValue(operationId, "kind", "s3");
            break;
        default:
            Y_DEBUG_ABORT_UNLESS(false, "Unknown import kind");
            break;
        }

        return operationId;
    }

    static Ydb::Operations::Operation ToOperation(const NKikimrImport::TImport& import) {
        Ydb::Operations::Operation operation;

        operation.set_id(NOperationId::ProtoToString(MakeOperationId(import.GetId(), import.GetSettingsCase())));
        operation.set_status(import.GetStatus());
        if (operation.status() == Ydb::StatusIds::SUCCESS) {
            operation.set_ready(import.GetProgress() == Ydb::Import::ImportProgress::PROGRESS_DONE);
        } else {
            operation.set_ready(true);
        }
        if (import.IssuesSize()) {
            operation.mutable_issues()->CopyFrom(import.GetIssues());
        }

        using namespace Ydb::Import;
        switch (import.GetSettingsCase()) {
        case NKikimrImport::TImport::kImportFromS3Settings:
            Fill<ImportFromS3Metadata, ImportFromS3Result>(operation, import, import.GetImportFromS3Settings());
            break;
        default:
            Y_DEBUG_ABORT_UNLESS(false, "Unknown import kind");
            break;
        }

        return operation;
    }

private:
    template <typename TMetadata, typename TResult, typename TSettings>
    static void Fill(
            Ydb::Operations::Operation& operation,
            const NKikimrImport::TImport& import,
            const TSettings& settings) {
        TMetadata metadata;
        metadata.mutable_settings()->CopyFrom(settings);
        metadata.set_progress(import.GetProgress());
        metadata.mutable_items_progress()->CopyFrom(import.GetItemsProgress());
        operation.mutable_metadata()->PackFrom(metadata);

        TResult result;
        operation.mutable_result()->PackFrom(result);
    }

}; // TImportConv

} // namespace NGRpcService
} // namespace NKikimr
