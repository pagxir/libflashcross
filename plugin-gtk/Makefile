NAME = plugin
PACKAGES = gtk+-2.0
CFLAGS = -DOSTYPE=\"FreeBSD8\" -DOSARCH=FreeBSD  -fPIC  \
	-fno-rtti -fno-exceptions -Wall -Wconversion -Wpointer-arith \
	-Woverloaded-virtual -Wsynth -Wno-ctor-dtor-privacy \
	-Wno-non-virtual-dtor -Wcast-align -Wno-long-long \
	-fno-strict-aliasing -fshort-wchar -pipe  -DNDEBUG \
	-I/usr/local/include/firefox \
	-I/usr/local/include/gtk-2.0 \
	-I/usr/local/include/glib-2.0 \
	-I/usr/local/include/cairo        \
	-I/usr/local/include/pango-1.0 \
	-I/usr/local/include/gdk-pixbuf-2.0 \
	-I/usr/local/include/atk-1.0 \
	-I/usr/local/include/ \
	-DTRIMMED -O -D_THREAD_SAFE  -DMOZILLA_CLIENT 

CXXFLAGS = `pkg-config --cflags $(PACKAGES)` -g -DXP_UNIX $(CFLAGS)
PLUGIN_OBJ = np_entry.o npn_gate.o npp_gate.o plugin.o \
	npp_script.o npp_class.o npp_object.o  des.o
LDFLAGS = -shared -Wl,-zdefs -Wl,-hlib$(NAME).so -Wl,-Bsymbolic \
	`pkg-config --libs $(PACKAGES)`

all: lib$(NAME).so

lib$(NAME).so: $(PLUGIN_OBJ)
	$(CXX) $(LDFLAGS) -o lib$(NAME).so $(PLUGIN_OBJ)
   
clean:
	rm -rf *.o

.SUFFIXES: .cpp .o

.cpp.o:
	$(CXX) -o $@ -c $(CXXFLAGS) $<

