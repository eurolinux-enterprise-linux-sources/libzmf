noinst_PROGRAMS = bmifuzzer zmffuzzer

AM_CXXFLAGS = -I$(top_srcdir)/inc \
	$(REVENGE_GENERATORS_CFLAGS) \
	$(REVENGE_CFLAGS) \
	$(REVENGE_STREAM_CFLAGS) \
	$(DEBUG_CXXFLAGS)

commonfuzzer_ldadd = \
	$(top_builddir)/src/lib/libzmf-@ZMF_MAJOR_VERSION@.@ZMF_MINOR_VERSION@.la \
	$(REVENGE_GENERATORS_LIBS) \
	$(REVENGE_LIBS) \
	$(REVENGE_STREAM_LIBS) \
	-lFuzzingEngine

bmifuzzer_LDADD = $(commonfuzzer_ldadd)

bmifuzzer_SOURCES = \
	bmifuzzer.cpp

zmffuzzer_LDADD = $(commonfuzzer_ldadd)

zmffuzzer_SOURCES = \
	zmffuzzer.cpp
