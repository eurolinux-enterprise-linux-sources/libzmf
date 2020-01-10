tests = test detection

check_PROGRAMS = $(tests)
check_LIBRARIES = libtest_driver.a

libtest_driver_a_CPPFLAGS = $(CPPUNIT_CFLAGS) $(DEBUG_CXXFLAGS)
libtest_driver_a_SOURCES = \
	test.cpp

test_CPPFLAGS = \
	-I$(top_srcdir)/inc \
	-I$(top_srcdir)/src/lib \
	$(REVENGE_CFLAGS) \
	$(CPPUNIT_CFLAGS) \
	$(DEBUG_CXXFLAGS)

test_LDFLAGS = -L$(top_builddir)/src/lib
test_LDADD = \
	libtest_driver.a \
	$(top_builddir)/src/lib/libzmf_internal.la \
	$(REVENGE_LIBS) \
	$(ICU_LIBS) \
	$(CPPUNIT_LIBS)

test_SOURCES = \
	ZMFTypesTest.cpp

detection_CPPFLAGS = \
	-DDETECTION_TEST_DIR=\"$(top_srcdir)/src/test/data\" \
	-I$(top_srcdir)/inc \
	$(REVENGE_STREAM_CFLAGS) \
	$(CPPUNIT_CFLAGS) \
	$(DEBUG_CXXFLAGS)

detection_LDFLAGS = -L$(top_builddir)/src/lib
detection_LDADD = \
	libtest_driver.a \
	$(top_builddir)/src/lib/libzmf-@ZMF_MAJOR_VERSION@.@ZMF_MINOR_VERSION@.la \
	$(REVENGE_STREAM_LIBS) \
	$(CPPUNIT_LIBS)

detection_SOURCES = \
	ZMFDocumentTest.cpp

TESTS = $(tests)

EXTRA_DIST = \
	data/bitmap.bmi \
	data/draw2.zmf \
	data/draw3.zmf \
	data/draw4.zmf \
	data/draw5.zmf \
	data/draw5-uncompressed.zmf \
	data/zebra.zbr
