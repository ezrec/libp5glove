INCLUDES=-I$(top_srcdir)/src

bin_PROGRAMS=p5dump p5hand

#noinst_PROGRAMS=p5data

COMMON=p5common.h p5common.c

p5dump_SOURCES=p5dump.c
p5dump_LDADD=$(top_builddir)/src/libp5glove.la 

p5hand_SOURCES=p5hand.c $(COMMON)
p5hand_LDADD=$(top_builddir)/src/libp5glove.la @GL_LIBS@ @X_LIBS@

#p5data_SOURCES=p5data.c $(COMMON)
#p5data_LDADD=$(top_builddir)/src/libp5glove.la -lm

