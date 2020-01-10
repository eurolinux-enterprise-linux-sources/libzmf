bin_PROGRAMS = zmf2svg

AM_CXXFLAGS = -I$(top_srcdir)/inc $(REVENGE_CFLAGS) $(REVENGE_GENERATORS_CFLAGS) $(REVENGE_STREAM_CFLAGS) $(DEBUG_CXXFLAGS)

zmf2svg_DEPENDENCIES = @ZMF2SVG_WIN32_RESOURCE@

zmf2svg_LDADD = ../../lib/libzmf-@ZMF_MAJOR_VERSION@.@ZMF_MINOR_VERSION@.la $(REVENGE_LIBS) $(REVENGE_GENERATORS_LIBS) $(REVENGE_STREAM_LIBS) @ZMF2SVG_WIN32_RESOURCE@ 

zmf2svg_SOURCES = \
	zmf2svg.cpp

if OS_WIN32

@ZMF2SVG_WIN32_RESOURCE@ : zmf2svg.rc $(zmf2svg_OBJECTS)
	chmod +x $(top_srcdir)/build/win32/*compile-resource
	WINDRES=@WINDRES@ $(top_srcdir)/build/win32/lt-compile-resource zmf2svg.rc @ZMF2SVG_WIN32_RESOURCE@
endif

EXTRA_DIST = \
	$(zmf2svg_SOURCES) \
	zmf2svg.rc.in

# These may be in the builddir too
BUILD_EXTRA_DIST = \
	zmf2svg.rc
