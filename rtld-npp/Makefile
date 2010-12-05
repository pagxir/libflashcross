CC = gcc
CXX = g++
PKGNAME = libxul nss
CFLAGS  = -g -fPIC -D__FREEBSD__ 
CXXFLAGS = $(CFLAGS) `pkg-config --cflags $(PKGNAME)` -I/usr/local/include 

LDLIBS = -Wl,-rpath,/home/pagx/plugins `pkg-config --libs nss`
LDLIBS += -lstdc++ -lssl -lm -lXt
RTLD_NAME = rtld
PLUGIN_NAME = flashcross
SONAME = lib$(PLUGIN_NAME).so

all: $(RTLD_NAME) $(SONAME)
	echo $(RTLD_NAME) $(SONAME)

OBJ_RTLD = rtld.o rtld_start.o rtld_fixup.o \
	linux.o flashsupport.o

$(RTLD_NAME): $(OBJ_RTLD)
	$(CC) $(LDFLAGS) -fPIC -o $(RTLD_NAME) $(OBJ_RTLD) $(LDLIBS)

OBJ_SOLIB = $(OBJ_RTLD) plugin.o

$(SONAME): $(OBJ_SOLIB)
	$(CC) -shared -Wl,-z,defs -Wl,-h,$(SONAME) $(LDFLAGS) -o $(SONAME) $(OBJ_SOLIB) $(LDLIBS) 

#$(CC) -shared -Wl,-z,defs -Wl,-h,$(SONAME) $(LDFLAGS) -o $(SONAME) $(OBJ_SOLIB) $(LDLIBS) -Wl,-Bsymbolic

clean:
	rm -f $(OBJ_RTLD) $(OBJ_SOLIB) $(SONAME) $(RTLD_NAME)
