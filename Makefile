.PHONY: all build check clean distcheck distproper mrproper output

THIRDPARTY := $(shell cd ../thirdparty && pwd)
STAGING := $(THIRDPARTY)/staging
TOOLS := $(THIRDPARTY)/tools

PKG_CONFIG_PATH := $(STAGING)/lib/pkgconfig:$(PKG_CONFIG_PATH)
CPPFLAGS := -I$(STAGING)/include $(CPPFLAGS)
PATH := $(STAGING)/bin:$(TOOLS)/bin:$(PATH)
LDFLAGS := -L$(STAGING)/lib $(LDFLAGS)
LC_ALL = C

debug := 0
CONFIGURE_FLAGS := --with-pic
ifeq ($(debug), 1)
CONFIGURE_FLAGS := ${CONFIGURE_FLAGS} --enable-debug
STRIP = true
else
CONFIGURE_FLAGS := ${CONFIGURE_FLAGS} --enable-silent-rules
STRIP = strip
endif

SYSTEM := $(shell uname -s)
ifeq ($(SYSTEM),Darwin)
SO := dylib
else
SO := so
endif

export CFLAGS CPPFLAGS CXXFLAGS LC_ALL LDFLAGS PATH PKG_CONFIG_PATH

MKFILES = configure.ac Makefile.am \
          flinter/Makefile.am \
          flinter/fastcgi/main/Makefile.am

cwd = $(shell pwd)

all: output
	$(MAKE) -C build dist-gzip
	$(MAKE) -C build dist-bzip2
	cp -f build/*.tar.* output

output: build
	rm -rf output
	$(MAKE) -C build install
	$(STRIP) -S output/lib/lib*.$(SO)
	cd output/lib && ln -s libflinter.a  libflinter_debug.a
	cd output/lib && ln -s libflinter.la libflinter_debug.la
	cd output/lib && ln -s libflinter.$(SO) libflinter_debug.$(SO)
	cd output/lib && ln -s libflinter_core.a  libflinter_core_debug.a
	cd output/lib && ln -s libflinter_core.la libflinter_core_debug.la
	cd output/lib && ln -s libflinter_core.$(SO) libflinter_core_debug.$(SO)
	cd output/lib && ln -s libflinter_fastcgi_main.a  libflinter_fastcgi_main_debug.a
	cd output/lib && ln -s libflinter_fastcgi_main.la libflinter_fastcgi_main_debug.la
	cd output/lib && ln -s libflinter_fastcgi_main.$(SO) libflinter_fastcgi_main_debug.$(SO)
	cd output/lib && ../../fixpath.sh libflinter.$(SO) '$$ORIGIN'
	cd output/lib && ../../fixpath.sh libflinter_core.$(SO) '$$ORIGIN'
	cd output/lib && ../../fixpath.sh libflinter_fastcgi_main.$(SO) '$$ORIGIN'

build: mrproper
	$(MAKE) -C build

clean:
	if [ -d dist ]; then chmod -R u+w dist; fi
	if [ -d build ]; then chmod -R u+w build; fi
	rm -rf output
	rm -rf dist
	rm -rf build
	rm -rf autom4te.cache
	rm -f m4/l* config.h.in~
	rm -f flinter/Makefile.in flinter/fastcgi/main/Makefile.in
	rm -f Makefile.in aclocal.m4 config.h.in configure config.guess
	rm -f compile depcomp install-sh missing ltmain.sh config.sub
	$(MAKE) -C test clean
	rm -rf test-bin

check: output
	$(MAKE) -C test check

distcheck: distproper
	$(MAKE) -C dist distcheck

distproper: dist/Makefile

dist/Makefile: configure
	rm -rf dist && mkdir -p dist
	cd dist && ../configure --prefix=$(cwd)/output

configure: $(MKFILES)
	./autogen.sh

mrproper: build/Makefile

build/Makefile: configure
	rm -rf build && mkdir -p build
	cd build && ../configure --prefix=$(cwd)/output $(CONFIGURE_FLAGS)
