DEPENDS(
    cloud/filestore/apps/client
    cloud/filestore/tests/recipes/vhost-endpoint
)

SET(RECIPE_ARGS --filesystem nfs_test)

IF (NOT OPENSOURCE)
    SET_APPEND(RECIPE_ARGS --verbose)
ENDIF()

IF (FILESTORE_SHARD_COUNT)
    SET_APPEND(RECIPE_ARGS --shard-count $FILESTORE_SHARD_COUNT)
ENDIF()

USE_RECIPE(
    cloud/filestore/tests/recipes/vhost-endpoint/vhost-endpoint-recipe
    ${RECIPE_ARGS}
)
