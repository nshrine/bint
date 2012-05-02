ACLOCAL_AMFLAGS = -I m4
SUBDIRS = lib tests

bin_PROGRAMS = int2bin bin2int illm2bin
noinst_HEADERS = bint.h getstrings.h illm.h
int2bin_SOURCES = int2bin.c bint.c getstrings.c
bin2int_SOURCES = bin2int.c bint.c getstrings.c
illm2bin_SOURCES = illm2bin.c bint.c

AM_CPPFLAGS = -I$(top_builddir)/lib -I$(top_srcdir)/lib
LDADD = lib/libgnu.a

man_MANS = int2bin.1 bin2int.1 illm2bin.1
EXTRA_DIST = $(man_MANS) m4/gnulib-cache.m4
MAINTAINERCLEANFILES = $(man_MANS)
common_mandeps = configure.ac

int2bin.1: int2bin.c $(common_mandeps)
	$(HELP2MAN) -N ./int2bin$(EXEEXT) -o $@
bin2int.1: bin2int.c $(common_mandeps)
	$(HELP2MAN) -N ./bin2int$(EXEEXT) -o $@
illm2bin.1: illm2bin.c $(common_mandeps)
	$(HELP2MAN) -N ./illm2bin$(EXEEXT) -o $@
