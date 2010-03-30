INCLUDES=-I$(top_srcdir)/src

bin_PROGRAMS=p5dump p5hand

p5dump_SOURCES=p5dump.c
p5dump_LDADD=$(top_builddir)/src/libp5glove.la 

p5hand_SOURCES=p5hand.c
p5hand_LDADD=$(top_builddir)/src/libp5glove.la @X_LIBS@
