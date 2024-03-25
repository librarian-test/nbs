LIBRARY()

SRCS(
    part.cpp
    part_actor.cpp
    part_actor_addblobs.cpp
    part_actor_addconfirmedblobs.cpp
    part_actor_addgarbage.cpp
    part_actor_addunconfirmedblobs.cpp
    part_actor_changedblocks.cpp
    part_actor_checkpoint.cpp
    part_actor_cleanup.cpp
    part_actor_collectgarbage.cpp
    part_actor_compaction.cpp
    part_actor_compactrange.cpp
    part_actor_confirmblobs.cpp
    part_actor_deletegarbage.cpp
    part_actor_describeblocks.cpp
    part_actor_flush.cpp
    part_actor_getusedblocks.cpp
    part_actor_initfreshblocks.cpp
    part_actor_initschema.cpp
    part_actor_loadstate.cpp
    part_actor_metadata_rebuild_blockcount.cpp
    part_actor_metadata_rebuild_usedblocks.cpp
    part_actor_metadata_rebuild.cpp
    part_actor_monitoring.cpp
    part_actor_monitoring_check.cpp
    part_actor_monitoring_compaction.cpp
    part_actor_monitoring_describe.cpp
    part_actor_monitoring_garbage.cpp
    part_actor_monitoring_view.cpp
    part_actor_patchblob.cpp
    part_actor_readblob.cpp
    part_actor_readblocks.cpp
    part_actor_scan_disk.cpp
    part_actor_statpartition.cpp
    part_actor_stats.cpp
    part_actor_trimfreshlog.cpp
    part_actor_waitready.cpp
    part_actor_writeblob.cpp
    part_actor_writeblocks.cpp
    part_actor_writefreshblocks.cpp
    part_actor_writemergedblocks.cpp
    part_actor_writemixedblocks.cpp
    part_actor_writequeue.cpp
    part_actor_zeroblocks.cpp
    part_counters.cpp
    part_database.cpp
    part_schema.cpp
    part_state.cpp
)

PEERDIR(
    cloud/blockstore/libs/common
    cloud/blockstore/libs/diagnostics
    cloud/blockstore/libs/kikimr
    cloud/blockstore/libs/storage/api
    cloud/blockstore/libs/storage/core
    cloud/blockstore/libs/storage/partition/model
    cloud/blockstore/libs/storage/partition_common
    cloud/blockstore/libs/storage/protos

    cloud/storage/core/libs/api
    cloud/storage/core/libs/common
    cloud/storage/core/libs/tablet
    cloud/storage/core/libs/viewer

    library/cpp/blockcodecs
    library/cpp/cgiparam
    library/cpp/containers/dense_hash
    library/cpp/lwtrace
    library/cpp/monlib/service/pages

    ydb/core/base
    ydb/core/node_whiteboard
    ydb/core/scheme
    ydb/core/tablet
    ydb/core/tablet_flat
    library/cpp/actors/core
)

END()

RECURSE(
    model
)

RECURSE_FOR_TESTS(
    ut
)
