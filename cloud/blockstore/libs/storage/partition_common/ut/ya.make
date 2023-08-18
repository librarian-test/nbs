UNITTEST_FOR(cloud/blockstore/libs/storage/partition_common)

INCLUDE(${ARCADIA_ROOT}/cloud/blockstore/tests/recipes/small.inc)

SRCS(
    actor_read_blob_ut.cpp
    actor_read_blocks_from_base_disk_ut.cpp
    drain_actor_companion_ut.cpp
)

PEERDIR(
    cloud/blockstore/libs/storage/testlib
)

YQL_LAST_ABI_VERSION()

END()
