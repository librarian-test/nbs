GO_LIBRARY()

LICENSE(BSD-3-Clause)

SRCS(
    jira.go
)

GO_TEST_SRCS(jira_test.go)

END()

RECURSE(
    gotest
)
