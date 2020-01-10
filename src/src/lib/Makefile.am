if PLATFORM_WIN32
version_info = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) -avoid-version
else
version_info = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
endif

noinst_LTLIBRARIES = libzmf_internal.la
lib_LTLIBRARIES = libzmf-@ZMF_MAJOR_VERSION@.@ZMF_MINOR_VERSION@.la

AM_CXXFLAGS = -I$(top_srcdir)/inc $(REVENGE_CFLAGS) $(REVENGE_STREAM_CFLAGS) $(ZLIB_CFLAGS) $(LIBPNG_CFLAGS) $(ICU_CFLAGS) $(DEBUG_CXXFLAGS) -DLIBZMF_BUILD

libzmf_@ZMF_MAJOR_VERSION@_@ZMF_MINOR_VERSION@_la_LIBADD = libzmf_internal.la $(REVENGE_LIBS) $(ZLIB_LIBS) $(LIBPNG_LIBS) $(ICU_LIBS) @LIBZMF_WIN32_RESOURCE@
libzmf_@ZMF_MAJOR_VERSION@_@ZMF_MINOR_VERSION@_la_DEPENDENCIES = libzmf_internal.la @LIBZMF_WIN32_RESOURCE@
libzmf_@ZMF_MAJOR_VERSION@_@ZMF_MINOR_VERSION@_la_LDFLAGS = $(version_info) -export-dynamic -no-undefined
libzmf_@ZMF_MAJOR_VERSION@_@ZMF_MINOR_VERSION@_la_SOURCES = \
	ZMFDocument.cpp

libzmf_internal_la_SOURCES = \
	BMIHeader.cpp \
	BMIHeader.h \
	BMIParser.cpp \
	BMIParser.h \
	BMITypes.cpp \
	BMITypes.h \
	ZBRHeader.cpp \
	ZBRHeader.h \
	ZBRParser.cpp \
	ZBRParser.h \
	ZMF2Header.cpp \
	ZMF2Header.h \
	ZMF2Parser.cpp \
	ZMF2Parser.h \
	ZMF4Header.cpp \
	ZMF4Header.h \
	ZMF4Parser.cpp \
	ZMF4Parser.h \
	ZMFCollector.cpp \
	ZMFCollector.h \
	ZMFTypes.cpp \
	ZMFTypes.h \
	libzmf_utils.cpp \
	libzmf_utils.h

if OS_WIN32

@LIBZMF_WIN32_RESOURCE@ : libzmf.rc $(libzmf_@ZMF_MAJOR_VERSION@_@ZMF_MINOR_VERSION@_la_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource libzmf.rc @LIBZMF_WIN32_RESOURCE@

endif


EXTRA_DIST = \
	libzmf.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	libzmf.rc
