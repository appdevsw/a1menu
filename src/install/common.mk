SRC=./src
SOURCES = $(wildcard $(SRC)/*.cpp)
HEADERS = $(wildcard $(SRC)/*.h)
CPP=g++
LDLIBS=-lmate-panel-applet-4 -lxml2 -lXi
LDPATHS=-L/usr/lib/x86_64-linux-gnu

INCLUDES=\
-I/usr/include/glib-2.0 \
-I/usr/include/libxml2 \
-I/usr/include/gdk-pixbuf-2.0/gdk-pixbuf \
-I/usr/include/mate-panel-4.0/libmate-panel-applet/


#dummy := $(shell mkdir -p $(TGT))



all: $(TGT)/a1menu-gtk.run createDEB createRPM

$(TGT)/%.o: $(SRC)/%.cpp $(HEADERS)
	@mkdir -p $(@D)
	$(CPP) $(CPPFLAGS) $(INCLUDES) -c $< -o $@

$(TGT)/a1menu-gtk.run: $(OBJS)
	$(CPP) $(LDFLAGS) $(LDPATHS) $(LDLIBS) $(OBJS) -o $@	

createRPM: $(TGT)/a1menu-gtk.run
	$(TGT)/a1menu-gtk.run --makerpm
	
createDEB: $(TGT)/a1menu-gtk.run
	$(TGT)/a1menu-gtk.run --makedeb
	