# TODO: move util.c and util.h to something like "common" or all that.
# TODO: rename "src" to "libionocore" or "core", this is not the main
#        source directory anymore.
# TODO: there are quite things that would be nice to remove, like
#        all the SDL-dependant tools. Fix this.

noinst_LIBRARIES = libionocore.a
libionocore_a_CFLAGS = -I. -I../libsao -I../common -I../block \
                       -ggdb -O0 -finline-functions
                       
libionocore_a_SOURCES = config_files.c \
                        config_files.h \
                        datasource.c \
                        datasource.h \
                        globe.c \
                        globe.h \
                        index.c \
                        index.h \
                        ionogram.c \
                        ionogram.h \
                        ionowatch.h \
                        misc.c \
                        libini.h \
                        inifile.c \
                        memfile.c \
                        libsim.h \
                        plugin.c \
                        plugin.h \
                        sunspot.c \
                        sunspot.h
                        
noinst_PROGRAMS = saocheck sao2mat

saocheck_LDADD = libionocore.a ../libsao/libsao.a ../block/libblock.a \
                  ../draw/libdraw.a ../wbmp/libwbmp.a ../common/libcommon.a
            
saocheck_LDFLAGS = -lm -lpthread -ldl -rdynamic @SDL_LIBS@

saocheck_CFLAGS = -I. -I../libsao -I../common -I../block  \
                   -ggdb -O3 -finline-functions


saocheck_SOURCES = main.c


#buildtset_LDADD = libionocore.a ../libsao/libsao.a ../block/libblock.a \
#                  ../draw/libdraw.a ../wbmp/libwbmp.a ../common/libcommon.a 
            
#buildtset_LDFLAGS = -lm -lpthread -ldl -rdynamic @SDL_LIBS@

#buildtset_CFLAGS = -I. -I../libsao -I../common -I../block \
#                   -ggdb -O0 -finline-functions


#buildtset_SOURCES = buildtset.c


sao2mat_LDADD = libionocore.a ../libsao/libsao.a ../block/libblock.a \
                 ../common/libcommon.a 
            
sao2mat_LDFLAGS = -lm -lpthread -ldl -rdynamic

sao2mat_CFLAGS = -I. -I../libsao -I../common -I../block \
                   -ggdb -O3 -finline-functions


sao2mat_SOURCES =   sao2mat.c



# saoday_LDADD =  libionocore.a ../libsao/libsao.a ../block/libblock.a \
#                  ../draw/libdraw.a ../wbmp/libwbmp.a ../common/libcommon.a 
            
# saoday_LDFLAGS = -lm -lpthread   -ldl -rdynamic @SDL_LIBS@

# saoday_CFLAGS = -I. -I../libsao -I../common -I../block -I../mlp -I../draw -I../wbmp \
#                  -ggdb -O3 -finline-functions


# saoday_SOURCES =    saoday.c


# test_SOURCES =   test.c



# test_LDADD =  libionocore.a ../libsao/libsao.a ../block/libblock.a \
#                   ../draw/libdraw.a ../wbmp/libwbmp.a ../common/libcommon.a  \
#                  ../mlp/libmlp.a 
            
# test_LDFLAGS = -lm -lpthread   -ldl -rdynamic @SDL_LIBS@

#test_CFLAGS = -I. -I../libsao -I../common -I../block -I../mlp -I../draw -I../wbmp \
#                   -ggdb -O3 -finline-functions







