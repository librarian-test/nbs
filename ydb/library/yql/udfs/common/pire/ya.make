YQL_UDF_YDB(pire_udf)

YQL_ABI_VERSION(
    2
    27
    0
)

SRCS(
    pire_udf.cpp
)

PEERDIR(
    library/cpp/regex/pire
)

END()

RECURSE_FOR_TESTS(
    test
)