NAME = wrapper
PACKAGES = gtk+-2.0
CFLAGS = -fPIC -g -DXP_UNIX -Iinclude \
	-fno-rtti -fno-exceptions -Wall -Wconversion -Wpointer-arith \
	-Woverloaded-virtual -Wsynth -Wno-ctor-dtor-privacy \
	-Wno-non-virtual-dtor -Wcast-align -Wno-long-long \
	-fno-strict-aliasing -fshort-wchar -pipe 

CXXFLAGS = `pkg-config --cflags $(PACKAGES)` $(CFLAGS)
PLUGIN_OBJ = np_entry.o
LDFLAGS = -shared -Wl,-zdefs -Wl,-hlib$(NAME).so -Wl,-Bsymbolic \
	`pkg-config --libs $(PACKAGES)` -L`pwd` -laliedit

all: lib$(NAME).so

lib$(NAME).so: $(PLUGIN_OBJ)
	$(CXX) $(LDFLAGS) -o lib$(NAME).so $(PLUGIN_OBJ)
   
clean:
	rm -rf *.o

.SUFFIXES: .cpp .o

.cpp.o:
	$(CXX) -o $@ -c $(CXXFLAGS) $<

