CC = gcc
CXX = g++
PKGNAME = gtk+-2.0
CFLAGS  = -g -fPIC -Iinclude 
CXXFLAGS = $(CFLAGS) `pkg-config --cflags $(PKGNAME)`

LDLIBS = `pkg-config --libs nss`
LDLIBS += -lstdc++ -lssl -lm -lXt
RTLD_NAME = rtld
PLUGIN_NAME = flashcross
SONAME = lib$(PLUGIN_NAME).so

all: $(RTLD_NAME) $(SONAME)
	echo $(RTLD_NAME) $(SONAME)

OBJ_RTLD = rtld.o rtld_start.o rtld_fixup.o \
	linux.o flashsupport.o

rtld_start.o: ${MACHINE_ARCH}/rtld_start.o

$(RTLD_NAME): $(OBJ_RTLD)
	$(CC) $(LDFLAGS) -fPIC -o $(RTLD_NAME) $(OBJ_RTLD) $(LDLIBS)

OBJ_SOLIB = $(OBJ_RTLD) plugin.o
#LDFLAGS += -Wl,-Bsymbolic

$(SONAME): $(OBJ_SOLIB)
	$(CC) -shared -Wl,-z,defs -Wl,-h,$(SONAME) $(LDFLAGS) -o $(SONAME) $(OBJ_SOLIB) $(LDLIBS) 

clean:
	rm -f $(OBJ_RTLD) $(OBJ_SOLIB) $(SONAME) $(RTLD_NAME)

