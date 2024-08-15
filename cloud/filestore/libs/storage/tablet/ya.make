LIBRARY()

GENERATE_ENUM_SERIALIZATION(tablet_private.h)
GENERATE_ENUM_SERIALIZATION(session.h)

SRCS(
    checkpoint.cpp
    helpers.cpp
    profile_log_events.cpp
    rebase_logic.cpp
    session.cpp
    subsessions.cpp
    tablet.cpp
    tablet_actor.cpp
    tablet_actor_accessnode.cpp
    tablet_actor_acquirelock.cpp
    tablet_actor_addblob.cpp
    tablet_actor_adddata.cpp
    tablet_actor_allocatedata.cpp
    tablet_actor_change_storage_config.cpp
    tablet_actor_cleanup.cpp
    tablet_actor_cleanupsessions.cpp
    tablet_actor_cluster.cpp
    tablet_actor_collectgarbage.cpp
    tablet_actor_compaction.cpp
    tablet_actor_compactionforced.cpp
    tablet_actor_counters.cpp
    tablet_actor_createcheckpoint.cpp
    tablet_actor_createhandle.cpp
    tablet_actor_createnode.cpp
    tablet_actor_createsession.cpp
    tablet_actor_delete_zero_compaction_ranges.cpp
    tablet_actor_deletecheckpoint.cpp
    tablet_actor_deletegarbage.cpp
    tablet_actor_destroycheckpoint.cpp
    tablet_actor_destroyhandle.cpp
    tablet_actor_destroysession.cpp
    tablet_actor_dumprange.cpp
    tablet_actor_filteralivenodes.cpp
    tablet_actor_flush.cpp
    tablet_actor_flush_bytes.cpp
    tablet_actor_generatecommitid.cpp
    tablet_actor_getnodeattr.cpp
    tablet_actor_getnodexattr.cpp
    tablet_actor_initschema.cpp
    tablet_actor_listnodes.cpp
    tablet_actor_listnodexattr.cpp
    tablet_actor_loadstate.cpp
    tablet_actor_monitoring.cpp
    tablet_actor_oplog.cpp
    tablet_actor_readblob.cpp
    tablet_actor_readdata.cpp
    tablet_actor_readlink.cpp
    tablet_actor_releaselock.cpp
    tablet_actor_removenodexattr.cpp
    tablet_actor_renamenode.cpp
    tablet_actor_request.cpp
    tablet_actor_resetsession.cpp
    tablet_actor_resolvepath.cpp
    tablet_actor_setnodeattr.cpp
    tablet_actor_setnodexattr.cpp
    tablet_actor_subscribesession.cpp
    tablet_actor_testlock.cpp
    tablet_actor_throttling.cpp
    tablet_actor_truncate.cpp
    tablet_actor_unlinknode.cpp
    tablet_actor_updateconfig.cpp
    tablet_actor_waitready.cpp
    tablet_actor_writebatch.cpp
    tablet_actor_writeblob.cpp
    tablet_actor_writedata.cpp
    tablet_actor_write_compactionmap.cpp
    tablet_actor_zerorange.cpp
    tablet_counters.cpp
    tablet_database.cpp
    tablet_private.cpp
    tablet_schema.cpp
    tablet_state.cpp
    tablet_state_cache.cpp
    tablet_state_channels.cpp
    tablet_state_checkpoints.cpp
    tablet_state_data.cpp
    tablet_state_nodes.cpp
    tablet_state_sessions.cpp
    tablet_state_throttling.cpp
    tablet_tx.cpp
)

PEERDIR(
    cloud/filestore/libs/diagnostics
    cloud/filestore/libs/diagnostics/metrics
    cloud/filestore/libs/service
    cloud/filestore/libs/storage/api
    cloud/filestore/libs/storage/core
    cloud/filestore/libs/storage/model
    cloud/filestore/libs/storage/tablet/actors
    cloud/filestore/libs/storage/tablet/model
    cloud/filestore/libs/storage/tablet/protos

    cloud/storage/core/libs/api
    cloud/storage/core/libs/common
    cloud/storage/core/libs/diagnostics
    cloud/storage/core/libs/tablet
    cloud/storage/core/libs/tablet/model
    cloud/storage/core/libs/viewer
    cloud/storage/core/protos

    contrib/ydb/library/actors/core
    library/cpp/protobuf/json

    contrib/ydb/core/base
    contrib/ydb/core/filestore/core
    contrib/ydb/core/mind
    contrib/ydb/core/node_whiteboard
    contrib/ydb/core/scheme
    contrib/ydb/core/tablet
    contrib/ydb/core/tablet_flat
)

END()

RECURSE(
    model
    protos
)

RECURSE_FOR_TESTS(
    ut
    ut_counters
    ut_stress
)
