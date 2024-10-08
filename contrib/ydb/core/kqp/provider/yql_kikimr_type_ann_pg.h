#pragma once
#include <contrib/ydb/library/yql/ast/yql_expr.h>
#include <contrib/ydb/library/yql/core/yql_expr_type_annotation.h>
#include <contrib/ydb/core/kqp/provider/yql_kikimr_expr_nodes.h>

namespace NYql {
namespace NPgTypeAnn {
    
bool IsPgInsert(
    const NNodes::TKiWriteTable& node, 
    TYdbOperation op);

bool ValidatePgUpdateKeys(
    const NNodes::TKiWriteTable& node, 
    const TKikimrTableDescription* table, 
    TExprContext& ctx);


} // namespace NPgTypeAnn
} // namespace NYql
