UNITTEST_FOR(contrib/ydb/core/sys_view/query_stats)

FORK_SUBTESTS()

SIZE(MEDIUM)

TIMEOUT(600)

PEERDIR(
    library/cpp/testing/unittest
    contrib/ydb/core/testlib/default
)

YQL_LAST_ABI_VERSION()

SRCS(
    query_stats_ut.cpp
)

END()
