RECURSE(
    python
    recipes
)

RECURSE_FOR_TESTS(
    build_arcadia_test
    client
    fio
    fio_index
    fio_index_migration
    fio_migration
    fs_posix_compliance
    loadtest
    profile_log
    registration
    service
    xfs_suite
)
