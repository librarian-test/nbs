UNITTEST_FOR(contrib/ydb/public/sdk/cpp/client/ydb_topic)

IF (SANITIZER_TYPE == "thread" OR WITH_VALGRIND)
    TIMEOUT(1200)
    SIZE(LARGE)
    TAG(ya:fat)
ELSE()
    TIMEOUT(600)
    SIZE(MEDIUM)
ENDIF()

FORK_SUBTESTS()

PEERDIR(
    library/cpp/testing/gmock_in_unittest
    contrib/ydb/core/testlib/default
    contrib/ydb/public/lib/json_value
    contrib/ydb/public/lib/yson_value
    contrib/ydb/public/sdk/cpp/client/ydb_driver
    contrib/ydb/public/sdk/cpp/client/ydb_persqueue_core
    contrib/ydb/public/sdk/cpp/client/ydb_persqueue_core/impl
    contrib/ydb/public/sdk/cpp/client/ydb_persqueue_core/ut/ut_utils
    contrib/ydb/public/sdk/cpp/client/ydb_topic/codecs

    contrib/ydb/public/sdk/cpp/client/ydb_topic
    contrib/ydb/public/sdk/cpp/client/ydb_topic/impl
    contrib/ydb/public/sdk/cpp/client/ydb_topic/ut/ut_utils
)

YQL_LAST_ABI_VERSION()

SRCS(
    basic_usage_ut.cpp
    describe_topic_ut.cpp
    local_partition_ut.cpp
    topic_to_table_ut.cpp
)

END()
