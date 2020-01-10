bin_PROGRAMS = zmf2raw

AM_CXXFLAGS = -I$(top_srcdir)/inc $(REVENGE_CFLAGS) $(REVENGE_GENERATORS_CFLAGS) $(REVENGE_STREAM_CFLAGS) $(DEBUG_CXXFLAGS)

zmf2raw_DEPENDENCIES = @ZMF2RAW_WIN32_RESOURCE@

zmf2raw_LDADD = ../../lib/libzmf-@ZMF_MAJOR_VERSION@.@ZMF_MINOR_VERSION@.la $(REVENGE_LIBS) $(REVENGE_GENERATORS_LIBS) $(REVENGE_STREAM_LIBS) @ZMF2RAW_WIN32_RESOURCE@ 

zmf2raw_SOURCES = \
	zmf2raw.cpp

if OS_WIN32

@ZMF2RAW_WIN32_RESOURCE@ : zmf2raw.rc $(zmf2raw_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource zmf2raw.rc @ZMF2RAW_WIN32_RESOURCE@
endif

EXTRA_DIST = \
	$(zmf2raw_SOURCES) \
	zmf2raw.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	zmf2raw.rc	 
