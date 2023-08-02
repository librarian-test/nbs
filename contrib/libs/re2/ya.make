# Generated by devtools/yamaker from nixpkgs 22.11.

LIBRARY()

LICENSE(
    BSD-3-Clause AND
    X11-Lucent
)

LICENSE_TEXTS(.yandex_meta/licenses.list.txt)

VERSION(2023-08-01)

ORIGINAL_SOURCE(https://github.com/google/re2/archive/2023-08-01.tar.gz)

PEERDIR(
    contrib/restricted/abseil-cpp/absl/base
    contrib/restricted/abseil-cpp/absl/container
    contrib/restricted/abseil-cpp/absl/hash
    contrib/restricted/abseil-cpp/absl/strings
    contrib/restricted/abseil-cpp/absl/synchronization
    library/cpp/sanitizer/include
)

ADDINCL(
    GLOBAL contrib/libs/re2/include
    contrib/libs/re2
)

NO_COMPILER_WARNINGS()

IF (WITH_VALGRIND)
    CFLAGS(
        GLOBAL -DRE2_ON_VALGRIND
    )
ENDIF()

SRCS(
    re2/bitmap256.cc
    re2/bitstate.cc
    re2/compile.cc
    re2/dfa.cc
    re2/filtered_re2.cc
    re2/mimics_pcre.cc
    re2/nfa.cc
    re2/onepass.cc
    re2/parse.cc
    re2/perl_groups.cc
    re2/prefilter.cc
    re2/prefilter_tree.cc
    re2/prog.cc
    re2/re2.cc
    re2/regexp.cc
    re2/set.cc
    re2/simplify.cc
    re2/tostring.cc
    re2/unicode_casefold.cc
    re2/unicode_groups.cc
    util/rune.cc
    util/strutil.cc
)

END()

RECURSE(
    re2/testing
)
