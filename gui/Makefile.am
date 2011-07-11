bin_PROGRAMS = ionowatch

ionowatch_CFLAGS = -I. -I../libsao -I../common -I../block -I../mlp \
                       -I../src -ggdb -O3 -finline-functions @GTK_CFLAGS@ \
                       @GTKGL_CFLAGS@ @GL_CFLAGS@ @GLU_CFLAGS@ -I../wbmp

ionowatch_LDFLAGS = -lm -lpthread @GTK_LIBS@ @GTKGL_LIBS@ @GL_LIBS@ \
                     @GLU_LIBS@ -export-dynamic  -ldl -rdynamic

ionowatch_LDADD = ../libsao/libsao.a ../block/libblock.a \
                  ../common/libcommon.a ../src/libionocore.a \
                  ../wbmp/libwbmp.a
            
ionowatch_SOURCES = callbacks.c common.c default_painter.c dialog.c filetree.c gui.h gui.c guisetup.c glglobe.c ionogram.c main.c message.c mufgraph.c painter.h painter.c scaled.c select.c trig.c trig.h glglobe.h gui.h ionograph.h message.h mufgraph.h tree.h

