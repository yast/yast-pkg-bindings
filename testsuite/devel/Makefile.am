#
# Makefile.am for testsuite/devel
#
INCLUDES= -I$(top_srcdir)/src -I$(includedir)

LDADD = $(top_builddir)/src/libpy2Pkg.la

AM_LDFLAGS = -static

CLEANFILES = *.bin

EXTRA_PROGRAMS = \
        wfm_test.bin

wfm_test_bin_SOURCES = wfm_test.cc
wfm_test_bin_LDADD = $(LDADD)

ALL: $(EXTRA_PROGRAMS)
