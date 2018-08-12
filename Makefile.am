
AUTOMAKE_OPTIONS = foreign

CXXFLAGS=
if DEBUG
AM_CXXFLAGS= -Wall -std=gnu++17 -g3
else
AM_CXXFLAGS= -Wall -std=gnu++17 -Ofast -DNDEBUG
endif

lib_LIBRARIES= libdsarch.a
libdsarch_a_SOURCES=dsarch.cc

EXTRA_DIST= dsarch.hh dsarch_types.hh

#
# Testing
# 

TESTS= dsarch_tests

check_PROGRAMS= dsarch_tests

EXTRA_dsarch_tests_SOURCES= dsarch_tests.hh
dsarch_tests_SOURCES= dsarch_tests.cc
dsarch_tests_LDADD= libdsarch.a

BUILT_SOURCES = dsarch_tests.cc
MAINTAINERCLEANFILES = dsarch_tests.cc

dsarch_tests.cc: $(EXTRA_dsarch_tests_SOURCES)
	@echo CXXFLAGS= $(CXXFLAGS)
	cxxtestgen --runner=ErrorPrinter -o $@ $<

# documentation
@DX_RULES@
