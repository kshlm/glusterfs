noinst_HEADERS = zmq-trans.h

rpctransport_LTLIBRARIES = zmq-trans.la
rpctransportdir = $(libdir)/glusterfs/$(PACKAGE_VERSION)/rpc-transport

zmq_trans_la_LDFLAGS = -module -avoid-version $(ZMQ_LDFLAGS)

zmq_trans_la_SOURCES = zmq-trans.c
zmq_trans_la_LIBADD = $(top_builddir)/libglusterfs/src/libglusterfs.la

AM_CPPFLAGS = $(GF_CPPFLAGS) \
	-I$(top_srcdir)/libglusterfs/src -I$(top_srcdir)/rpc/rpc-lib/src/ \
	-I$(top_srcdir)/rpc/xdr/src/

AM_CFLAGS = -Wall $(GF_CFLAGS) $(ZMQ_CFLAGS)

CLEANFILES = *~
