GO_LIBRARY()

LICENSE(BSD-3-Clause)

SRCS(
    clientcredentials.go
)

GO_TEST_SRCS(clientcredentials_test.go)

END()

RECURSE(
    gotest
)
