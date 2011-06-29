NAME = plugin
PACKAGES = gtk+-2.0
CFLAGS = -fPIC -O -g -Iinclude \
	-fno-exceptions -Wall -Wconversion -Wpointer-arith \
	-Wcast-align -Wno-long-long \
	-fno-strict-aliasing -fshort-wchar -pipe 

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

