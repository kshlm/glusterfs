noinst_HEADERS = zmq.h name.h

rpctransport_LTLIBRARIES = zmq.la
rpctransportdir = $(libdir)/glusterfs/$(PACKAGE_VERSION)/rpc-transport

zmq_la_LDFLAGS = -module -avoid-version

zmq_la_SOURCES = zmq.c name.c
zmq_la_LIBADD = $(top_builddir)/libglusterfs/src/libglusterfs.la -lzmq

AM_CPPFLAGS = $(GF_CPPFLAGS) \
	-I$(top_srcdir)/libglusterfs/src -I$(top_srcdir)/rpc/rpc-lib/src/ \
	-I$(top_srcdir)/rpc/xdr/src/

AM_CFLAGS = -Wall $(GF_CFLAGS)

CLEANFILES = *~
