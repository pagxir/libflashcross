CC = wine /media/system/Applications/MinGW/bin/gcc.exe
CXX = wine /media/system/Applications/MinGW/bin/g++.exe
WINDRES = wine /media/system/Applications/MinGW/bin/windres.exe

CC = gcc
CXX = g++
WINDRES = windres

CFLAGS +=  -O2 -g -DVIEWER -Dstricmp=strcasecmp
CXXFLAGS += -Iinclude -Iinclude/jni $(CFLAGS)

LDFLAGS += -L.
LDLIBS += -lgdi32 -lws2_32 -lnpswf32
LDFLAGS = -lm -L. -lgdi32 -lws2_32 
OBJECTS = shell.o shellrc.o netscape.o npp_script.o \
		  npp_class.o npp_object.o rpcvm.o npp_window.o \
		  npnetstream.o nputils.o npplugin.o

flash.exe: $(OBJECTS)
	$(CXX) $(LDFLAGS) $(OBJECTS) -g -o $@ $(LDLIBS)

shellrc.o: shell.rc
	$(WINDRES) -i shell.rc -o shellrc.o

clean:
	rm -f $(OBJECTS)

