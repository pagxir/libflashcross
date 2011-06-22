PACKAGES = gtk+-2.0 libxul nspr
CXXFLAGS = `pkg-config --cflags $(PACKAGES)` -g -DXP_UNIX \
	-I/usr/local/include/ \
	-I/usr/local/include/firefox \
	-I/usr/local/include/gtk-2.0 \
	-I/usr/local/include/atk-1.0 \
	-I/usr/local/include/cairo \
	-I/usr/local/include/gdk-pixbuf-2.0 \
	-I/usr/local/include/pango-1.0 \
	-I/usr/local/include/glib-2.0

XULBOX_OBJ = npprunner.o netscape.o npp_class.o \
	npp_object.o npp_script.o

LDLIBS = `pkg-config --libs $(PACKAGES)` \
	-L/usr/local/lib \
	-L/usr/lib \
	-lgtk-x11-2.0

all: xulbox

xulbox: $(XULBOX_OBJ)
	c++ -o xulbox  $(XULBOX_OBJ) -Wl,-Bsymbolic \
		-L/usr/local/lib -L. -lflashcross $(LDLIBS)
   
clean:
	rm -rf *.o

.SUFFIXES: .cpp .o

.cpp.o:
	$(CXX) -o $@ -c $(CXXFLAGS) -DOSTYPE=\"FreeBSD7\" -DOSARCH=FreeBSD  -I. -I/usr/include  -fPIC  -fno-rtti -fno-exceptions -Wall -Wconversion -Wpointer-arith -Woverloaded-virtual -Wsynth -Wno-ctor-dtor-privacy -Wno-non-virtual-dtor -Wcast-align -Wno-long-long -fno-strict-aliasing -fshort-wchar -pipe  -DNDEBUG -DTRIMMED -O -D_THREAD_SAFE  -DMOZILLA_CLIENT $<
