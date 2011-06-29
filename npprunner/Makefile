PACKAGES = gtk+-2.0
CXXFLAGS = -g -DXP_UNIX \
	-Iinclude -I/usr/local/include \
	`pkg-config --cflags $(PACKAGES)`

XULBOX_OBJ = npprunner.o netscape.o npp_class.o \
	npp_object.o npp_script.o npn_network.o \
	npn_stream.o

LDLIBS = -L/usr/local/lib \
	`pkg-config --libs $(PACKAGES)`

all: xulbox

xulbox: $(XULBOX_OBJ)
	c++ -o xulbox  $(XULBOX_OBJ) -Wl,-Bsymbolic -L. -lflashcross $(LDLIBS)
   
clean:
	rm -rf *.o

.SUFFIXES: .cpp .o

.cpp.o:
	$(CXX) -o $@ -c $(CXXFLAGS) -fPIC  -fno-rtti -fno-exceptions -Wall \
		-Wconversion -Wpointer-arith -Woverloaded-virtual -Wsynth \
		-Wno-ctor-dtor-privacy -Wno-non-virtual-dtor -Wcast-align \
		-Wno-long-long -fno-strict-aliasing -fshort-wchar -pipe  -O $<

