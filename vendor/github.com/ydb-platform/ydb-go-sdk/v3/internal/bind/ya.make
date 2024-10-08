GO_LIBRARY()

LICENSE(Apache-2.0)

SRCS(
    auto_declare.go
    bind.go
    errors.go
    numeric_args.go
    params.go
    positional_args.go
    sql_lexer.go
    table_path_prefix.go
)

GO_TEST_SRCS(
    bind_test.go
    numeric_args_test.go
    params_test.go
    positional_args_test.go
)

END()

RECURSE(
    gotest
)
