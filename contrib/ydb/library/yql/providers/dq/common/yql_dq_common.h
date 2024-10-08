#pragma once

#include <contrib/ydb/library/yql/dq/actors/dq.h>

#include <util/generic/string.h>
#include <map>

namespace NYql {
namespace NCommon {

TMaybe<TString> SqlToSExpr(const TString& query);

TString GetSerializedResultType(const TString& program);

bool ParseCounterName(TString* prefix, std::map<TString, TString>* labels, TString* name, const TString& counterName);

bool IsRetriable(NYql::NDqProto::StatusIds::StatusCode statusCode);
bool IsRetriable(const NDq::TEvDq::TEvAbortExecution::TPtr& ev);
bool NeedFallback(NYql::NDqProto::StatusIds::StatusCode statusCode);
bool NeedFallback(const NDq::TEvDq::TEvAbortExecution::TPtr& ev);
} // namespace NCommon
} // namespace NYql
