SUBDIRS=mlp
noinst_PROGRAMS = buildtset ionotrainer pftrainer h2pftrainer
inst_LTLIBRARIES=libmlpfull.la

libmlpfull_la_SOURCES=datafile.c datafile.h evaluators.c evaluators.h h2pftrainer.c id3.c id3.h id3syntax.y ionotrainer.c jit.c jit.h pftrainer.c tree.c trainer.c trainer.h trainthread.c tree.h mlpfull.c normal.h
libmlpfull_la_CFLAGS = -g -O3 -I. -I../../src -Imlp -I../../gui -I../../common -I../../libsao -I../../wbmp -I../../block -DPLUGIN_CODE @GTK_CFLAGS@ @GTKGL_CFLAGS@ @GL_CFLAGS@ @GLU_CFLAGS@
libmlpfull_la_LDFLAGS=@GTK_LIBS@ @GTKGL_LIBS@ @GL_LIBS@ @GLU_LIBS@ -rdynamic -shared -fPIC -Lmlp
libmlpfull_la_LIBADD=-ldl -lmlp

instdir=/usr/lib/ionowatch/plugins


TOOLS_LDADD = ../../src/libionocore.a ../../libsao/libsao.a ../../block/libblock.a \
              ../../common/libcommon.a  mlp/libmlp.a 
                  

TOOLS_CFLAGS = -I. -I../../libsao -I../../common -I../../block -Imlp \
                   -ggdb -O3 -finline-functions -Wall 

TOOLS_SRC = trainer.c trainer.h trainapp.c normal.h

ionotrainer_SOURCES = ionotrainer.c $(TOOLS_SRC) 

ionotrainer_LDADD = $(TOOLS_LDADD)
            
ionotrainer_LDFLAGS = -lm -lpthread  -ldl -rdynamic @SDL_LIBS@

ionotrainer_CFLAGS = $(TOOLS_CFLAGS)


pftrainer_SOURCES = pftrainer.c $(TOOLS_SRC)

pftrainer_LDADD = $(TOOLS_LDADD)
            
pftrainer_LDFLAGS = -lm -lpthread  -ldl -rdynamic @SDL_LIBS@

pftrainer_CFLAGS = $(TOOLS_CFLAGS)


h2pftrainer_SOURCES = h2pftrainer.c $(TOOLS_SRC)

h2pftrainer_LDADD = $(TOOLS_LDADD)
            
h2pftrainer_LDFLAGS = -lm -lpthread  -ldl -rdynamic @SDL_LIBS@

h2pftrainer_CFLAGS = $(TOOLS_CFLAGS)


buildtset_SOURCES = buildtset.c

buildtset_LDADD = $(TOOLS_LDADD)

buildtset_LDFLAGS = -lm -lpthread  -ldl -rdynamic

buildtset_CFLAGS = $(TOOLS_CFLAGS)

