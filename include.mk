# Copyright 2014 yiyuanzhong@gmail.com (Yiyuan Zhong)
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


# Optional parameters.
#
# ReleaseAsDebug        : release builds are simply symlinks to debug builds.
# EnableProfiling       : debug builds with profiling enabled.
# StaticLinkLibraries   : try to link static libraries (without glibc).
# DisableStackProtector : don't engage stack protectors.

# Workaround for make 3.81 and below.
_lastword = $(if $(firstword $1),$(word $(words $1),$1))
_self := $(dir $(call _lastword,$(MAKEFILE_LIST)))
_abspath = $(shell cd "$(1)" && pwd)
_pwd := $(shell pwd)

# Calculate $(MODULEROOT)
ifeq ($(MODULEROOT),)
ifeq ($(_self),./)
MODULEROOT := ..
else
MODULEROOT := $(patsubst %/flinter/,%,$(_self))
endif
endif

SUBDIRS := $(sort . $(SUBDIRS))
CGI_SUFFIX ?= .cgi

# Translate sources to objects.
make_release_objects = \
        $(foreach f,$(filter %.c %.cpp,$(1)),$(dir $(f)).libs/$(notdir $(f)).release.o) \
        $(foreach f,$(filter %.proto,$(1)),$(dir $(f)).libs/$(basename $(notdir $(f))).pb.cc.release.o)

make_debug_objects = \
        $(foreach f,$(filter %.c %.cpp,$(1)),$(dir $(f)).libs/$(notdir $(f)).debug.o) \
        $(foreach f,$(filter %.proto,$(1)),$(dir $(f)).libs/$(basename $(notdir $(f))).pb.cc.debug.o)

nolibs   = $(patsubst %.libs/,%,$(dir $(1)))$(notdir $(1))
clean    = $(patsubst %.libs/../,%,$(dir $(1)))$(notdir $(1))
cleantft = $(patsubst %gen-cpp/../,%,$(dir $(1)))$(notdir $(1))
cleandir = $(dir $(patsubst %.libs/../$(notdir $(1)),%$(notdir $(1)),$(1)))

PROTOS  = $(patsubst ./%,%,$(foreach subdir,$(SUBDIRS),$(wildcard $(subdir)/*.proto)))
THRIFTS = $(patsubst ./%,%,$(foreach subdir,$(SUBDIRS),$(wildcard $(subdir)/*.thrift)))
SOURCES_THRIFT_ALL = $(foreach f,$(sort $(dir $(THRIFTS))),$(wildcard $(f)gen-cpp/*.cpp))
SOURCES_THRIFT_SKIP = $(foreach f,$(addsuffix gen-cpp,$(dir $(THRIFTS))),$(wildcard $(f)/*_server.skeleton.cpp))
SOURCES_THRIFT = $(patsubst ./%,%,$(filter-out $(SOURCES_THRIFT_SKIP),$(SOURCES_THRIFT_ALL)))
SOURCES = $(patsubst ./%,%,$(foreach subdir,$(SUBDIRS),$(wildcard $(subdir)/*.c) $(wildcard $(subdir)/*.cpp))) $(SOURCES_THRIFT)

CGI_SOURCES = $(patsubst ./%,%,$(foreach subdir,$(SUBDIRS),$(wildcard $(subdir)/*_cgi.cpp)))
CGI = $(foreach cgi,$(CGI_SOURCES),$(TARGET)/$(patsubst ./%,%,$(dir $(cgi)))$(notdir $(patsubst %_cgi.cpp,%$(CGI_SUFFIX),$(cgi))))
CGId = $(foreach cgi,$(CGI_SOURCES),$(TARGET)/$(patsubst ./%,%,$(dir $(cgi)))$(notdir $(patsubst %_cgi.cpp,%_debug$(CGI_SUFFIX),$(cgi))))

TEST_SOURCES = $(patsubst ./%,%,$(foreach subdir,$(SUBDIRS),$(wildcard $(subdir)/test_*.cpp)))
TEST = $(foreach test,$(TEST_SOURCES),$(TARGET)/$(patsubst ./%,%,$(dir $(test)))$(notdir $(patsubst %.cpp,%,$(test))))
TESTd = $(foreach test,$(TEST_SOURCES),$(TARGET)/$(patsubst ./%,%,$(dir $(test)))$(notdir $(patsubst %.cpp,%_debug,$(test))))

CLEANS_REL = $(sort $(foreach subdir,$(SUBDIRS),$(subdir)/.libs/*.release.o) \
                    $(foreach subdir,$(SUBDIRS),$(subdir)/.libs/*.release.deps) \
                    $(foreach test,$(TEST_SOURCES),$(TARGET)/$(basename $(test))) \
                    $(GOAL))

CLEANS_DBG = $(sort $(foreach subdir,$(SUBDIRS),$(subdir)/.libs/*.debug.o) \
                    $(foreach subdir,$(SUBDIRS),$(subdir)/.libs/*.debug.deps) \
                    $(foreach test,$(TEST_SOURCES),$(TARGET)/$(basename $(test))_debug) \
                    $(GOALd))

ifeq ($(GOAL),$(EXECUTABLE))
	CLEANS_REL += $(GOAL).syms
endif

ifeq ($(GOAL),$(SHLIBRARY))
	CLEANS_REL += $(SHLIBRARYREAL)
endif

CLEANS = $(CLEANS_REL) $(CLEANS_DBG) .libs \
         $(foreach subdir,$(SUBDIRS),$(subdir)/gen-cpp) \
         $(foreach subdir,$(SUBDIRS),$(subdir)/*.pb.*) \
         $(foreach subdir,$(SUBDIRS),$(subdir)/.libs)

# Same as .libs/Makefile.dep, remember to synchronize.
.PRECIOUS: $(foreach proto,$(PROTOS),$(dir $(proto)).libs/../$(notdir $(basename $(proto))).pb.h) \
           $(foreach proto,$(PROTOS),$(dir $(proto)).libs/../$(notdir $(basename $(proto))).pb.cc) \
           $(foreach thrift,$(THRIFTS),$(dir $(thrift))gen-cpp/$(notdir $(basename $(thrift)))_types.cpp) \
           $(foreach thrift,$(THRIFTS),$(dir $(thrift))gen-cpp/$(notdir $(basename $(thrift)))_constants.cpp)

# Special treatments for CGIs.
CGI_COMMON ?= $(filter-out $(CGI_SOURCES) $(TEST_SOURCES),$(SOURCES))
CGI_OBJ_REL = $(call make_release_objects,$(CGI_COMMON) $(PROTOS) $(THRIFTS))
CGI_OBJ_DBG = $(call make_debug_objects,$(CGI_COMMON) $(PROTOS) $(THRIFTS))

# Special treatments for tests.
TEST_COMMON ?= $(filter-out $(CGI_SOURCES) $(TEST_SOURCES),$(SOURCES))
TEST_OBJ_REL = $(call make_release_objects,$(TEST_COMMON) $(PROTOS) $(THRIFTS))
TEST_OBJ_DBG = $(call make_debug_objects,$(TEST_COMMON) $(PROTOS) $(THRIFTS))

AR ?= ar
CC ?= gcc
CXX ?= g++
STRIP ?= strip
OBJCOPY ?= objcopy
AR := $(CROSS_COMPILE)$(AR)
CC := $(CROSS_COMPILE)$(CC)
CXX := $(CROSS_COMPILE)$(CXX)
STRIP := $(CROSS_COMPILE)$(STRIP)
OBJCOPY := $(CROSS_COMPILE)$(OBJCOPY)

MCY = mcy --language=cxx
THRIFT = thrift
PROTOC = protoc
PWD = pwd
CD = cd

XARGS = xargs --no-run-if-empty
PKGCONFIG = pkg-config
INSTALL = install
DIRNAME = dirname
MKDIR = mkdir -p
CHMOD = chmod
TOUCH = touch
EGREP = egrep
TRUE = true
SORT = sort
FIND = find
ECHO = echo
SVN = svn
SED = sed
AWK = awk
SH = sh
LN = ln -f
CP = cp -rf
RM = rm -rf

# Dealing with environment variants really hurts.
define prepend_path
    PATH := $(shell $(PWD))/$(MODULEROOT)/$(1):$(PATH)
endef
define prepend_ldpath
    LD_LIBRARY_PATH := $(shell $(PWD))/$(MODULEROOT)/$(1):$(LD_LIBRARY_PATH)
endef
$(foreach binpath,$(BINPATH),$(eval $(call prepend_path,$(binpath))))
$(foreach libpath,$(LIBRARYPATH),$(eval $(call prepend_ldpath,$(libpath))))
export PATH LD_LIBRARY_PATH

$(shell $(MKDIR) .libs)
$(foreach subdir,$(sort $(dir $(THRIFTS))),$(shell $(MKDIR) $(subdir)/gen-cpp/.libs))
$(foreach subdir,$(sort $(dir $(SOURCES) $(PROTOS))),$(shell $(MKDIR) $(subdir)/.libs))
CFLAGS_PKGCONFIG += $(foreach pkgconfig,$(PKGCONFIGS),$(shell $(PKGCONFIG) $(pkgconfig) --cflags 2>/dev/null))
LDFLAGS_PKGCONFIG += $(foreach pkgconfig,$(PKGCONFIGS),$(shell $(PKGCONFIG) $(pkgconfig) --libs 2>/dev/null))

CXXFLAGS_REL +=
CXXFLAGS_DBG +=
CXXFLAGS_ALL += -Woverloaded-virtual

CFLAGS_DBG += -DDEBUG -D_DEBUG -DDBG
CFLAGS_REL += -DNDEBUG -O2 -fomit-frame-pointer -Wuninitialized
CFLAGS_REL += -ffunction-sections -fdata-sections
CFLAGS_ALL += -Wall -Wextra -Wno-unused-parameter -Winit-self
CFLAGS_ALL += -g
CFLAGS_ALL += -pipe
CFLAGS_ALL += -fPIC
CFLAGS_ALL += -fno-strict-aliasing
CFLAGS_ALL += -DPREFIX=\"$(prefix)\"
CFLAGS_ALL += $(foreach path,$(SEARCHPATH),-I$(MODULEROOT)/$(path))

GCC_VER_MAJOR := $(shell $(CC) -dumpversion | $(AWK) -F. '{ print ($$1 >= 4) }')
GCC_VER_MINOR := $(shell $(CC) -dumpversion | $(AWK) -F. '{ print ($$1 >= 4 && $$2 >= 3) }')

ifeq ($(GCC_VER_MAJOR),1)
CFLAGS_ALL += -iquote$(MODULEROOT)

ifneq ($(strip $(DisableStackProtector)),1)
CFLAGS_ALL += -fstack-protector
endif

# Useful but anonying.
#CXXFLAGS_ALL += -Wsign-conversion

# Requires 4.3 or above.
ifeq ($(GCC_VER_MINOR),1)
CXXFLAGS_ALL += -std=gnu++0x
endif

else
CXXFLAGS_ALL += -Wconversion
CFLAGS_ALL += -I$(MODULEROOT)
endif

LDFLAGS_REL +=
LDFLAGS_DBG +=
LDFLAGS_ALL += $(foreach path,$(LIBRARYPATH),-L$(MODULEROOT)/$(path))

ifeq ($(strip $(EnableProfiling)),1)
CFLAGS_DBG += -pg
LDFLAGS_DBG += -pg
endif

ifeq ($(strip $(StaticLinkLibraries)),1)
LDFLAGS_LIB_S = $(foreach lib,$(LIBRARIES),$(foreach path,$(LIBRARYPATH),$(wildcard $(MODULEROOT)/$(path)/lib$(lib).a)))
LDFLAGS_LIB_F = $(filter-out $(patsubst lib%.a,%,$(foreach lib,$(LDFLAGS_LIB_S),$(notdir $(lib)))),$(LIBRARIES))
LDFLAGS_LIB = $(LDFLAGS_LIB_S) $(foreach lib,$(LDFLAGS_LIB_F),-l$(lib))
else
LDFLAGS_LIB = $(foreach lib,$(LIBRARIES),-l$(lib))
endif

OBJS_REL = $(sort $(call make_release_objects,$(SOURCES) $(PROTOS) $(THRIFTS)))
OBJS_DBG = $(sort $(call make_debug_objects,$(SOURCES) $(PROTOS) $(THRIFTS)))
DEPS_REL = $(foreach source,$(SOURCES),$(dir $(source)).libs/$(notdir $(source)).release.deps)
DEPS_DBG = $(foreach source,$(SOURCES),$(dir $(source)).libs/$(notdir $(source)).debug.deps)

STATICS_REL = $(foreach static,$(STATICS),$(MODULEROOT)/$(dir $(static))lib$(notdir $(static)).a)
STATICS_DBG = $(foreach static,$(STATICS),$(MODULEROOT)/$(dir $(static))lib$(notdir $(static))_debug.a)

STATICS_DEPS_REL = $(foreach static,$(STATICS),$(MODULEROOT)/$(dir $(static))lib$(notdir $(static)).a)
STATICS_DEPS_DBG = $(foreach static,$(STATICS),$(MODULEROOT)/$(dir $(static))lib$(notdir $(static))_debug.a)

# Finalize
LDFLAGS_REL_FINAL  = $(LDFLAGS_REL) $(LDFLAGS_ALL) $(LDFLAGS) $(LDFLAGS_PKGCONFIG)
LDFLAGS_DBG_FINAL  = $(LDFLAGS_DBG) $(LDFLAGS_ALL) $(LDFLAGS) $(LDFLAGS_PKGCONFIG)
CFLAGS_REL_FINAL   = $(CPPFLAGS) $(CFLAGS_REL) $(CFLAGS_ALL) $(CFLAGS) $(CFLAGS_PKGCONFIG)
CFLAGS_DBG_FINAL   = $(CPPFLAGS) $(CFLAGS_DBG) $(CFLAGS_ALL) $(CFLAGS) $(CFLAGS_PKGCONFIG)
CXXFLAGS_REL_FINAL = $(CPPFLAGS) $(CXXFLAGS_REL) $(CXXFLAGS_ALL) $(CXXFLAGS) $(CFLAGS_REL) $(CFLAGS_ALL) $(CFLAGS) $(CFLAGS_PKGCONFIG)
CXXFLAGS_DBG_FINAL = $(CPPFLAGS) $(CXXFLAGS_DBG) $(CXXFLAGS_ALL) $(CXXFLAGS) $(CFLAGS_DBG) $(CFLAGS_ALL) $(CFLAGS) $(CFLAGS_PKGCONFIG)

EXECUTABLE = $(TARGET)
EXECUTABLEd = $(TARGET)_debug

LIBRARY = $(patsubst ./%,%,$(dir $(TARGET)))lib$(basename $(notdir $(TARGET))).a
LIBRARYd = $(patsubst ./%,%,$(dir $(TARGET)))lib$(basename $(notdir $(TARGET)))_debug.a
SHLIBRARY = $(patsubst ./%,%,$(dir $(TARGET)))lib$(basename $(notdir $(TARGET))).so.0
SHLIBRARYREAL = $(patsubst ./%,%,$(dir $(TARGET)))lib$(basename $(notdir $(TARGET))).so
SHLIBRARYd = $(patsubst ./%,%,$(dir $(TARGET)))lib$(basename $(notdir $(TARGET)))_debug.so
SHLIBRARYNAME = lib$(basename $(notdir $(TARGET))).so.0
SHLIBRARYREALNAME = lib$(basename $(notdir $(TARGET))).so

COLORFUL = 0
ifeq ($(TERM),linux)
COLORFUL = 1
endif
ifeq ($(TERM),xterm)
COLORFUL = 1
endif
ifeq ($(TERM),screen)
COLORFUL = 1
endif

ifneq ($(COLORFUL),1)
ECHO_LDR = $(ECHO) '   LD[R]   '
ECHO_LDD = $(ECHO) '   LD[D]   '
ECHO_CCR = $(ECHO) '   CC[R]   '
ECHO_CCD = $(ECHO) '   CC[D]   '
ECHO_ARR = $(ECHO) '   AR[R]   '
ECHO_ARD = $(ECHO) '   AR[D]   '
ECHO_PBC = $(ECHO) '   PROTOC  '
ECHO_TFT = $(ECHO) '   THRIFT  '
else
ECHO_LDR = $(ECHO) '   [;32mLD[R][0m   '
ECHO_LDD = $(ECHO) '   [;32mLD[D][0m   '
ECHO_CCR = $(ECHO) '   [;33mCC[R][0m   '
ECHO_CCD = $(ECHO) '   [;33mCC[D][0m   '
ECHO_ARR = $(ECHO) '   [;31mAR[R][0m   '
ECHO_ARD = $(ECHO) '   [;31mAR[D][0m   '
ECHO_PBC = $(ECHO) '   [;35mPROTOC[0m  '
ECHO_TFT = $(ECHO) '   [;35mTHRIFT[0m  '
endif

.PHONY: mrproper release debug all clean debug-clean release-clean headers check shell
.PHONY: release-submakes debug-submakes

release: mrproper release-submakes $(GOAL)
debug: mrproper debug-submakes $(GOALd)

define submakes-target
release-submakes-$(1):
	@$$(MAKE) -C $(1) release
debug-submakes-$(1):
	@$$(MAKE) -C $(1) debug
clean-submakes-$(1):
	@$$(MAKE) -C $(1) clean
release-clean-submakes-$(1):
	@$$(MAKE) -C $(1) release-clean
debug-clean-submakes-$(1):
	@$$(MAKE) -C $(1) debug-clean
endef

$(foreach sm,$(SUBMAKES),$(eval $(call submakes-target,$(sm))))

release-submakes: $(addprefix release-submakes-,$(SUBMAKES))
debug-submakes: $(addprefix debug-submakes-,$(SUBMAKES))

all: release debug
check: release debug
	@$(ECHO) $(foreach test,$(TESTd) $(TEST),$(test)) | $(XARGS) -r -n1 $(SH) -c

mrproper: .PRECIOUS

distclean: clean
	@-$(SVN) status | $(AWK) '{ if ($$1 != "?") { print substr($$0, 9); } }' | $(SORT) -r | $(XARGS) $(SVN) revert --depth immediates
	@-$(SVN) status --no-ignore | $(AWK) '{ print substr($$0, 9); }' | $(XARGS) $(RM)

clean: $(addprefix clean-submakes-,$(SUBMAKES))
	@-$(RM) $(CLEANS) || $(RM) $(CLEANS)

release-clean: $(addprefix release-clean-submakes-,$(SUBMAKES))
	@-$(RM) $(CLEANS_REL) || $(RM) $(CLEANS_REL)

debug-clean: $(addprefix debug-clean-submakes-,$(SUBMAKES))
	@-$(RM) $(CLEANS_DBG) || $(RM) $(CLEANS_DBG)

ifeq ($(MODULEPATH),)
headers:
	@echo 'You must set $$(MODULEPATH)'
	@false
else
headers:
	@$(RM) include
	@for i in `$(FIND) . | $(EGREP) '\.h$$'`; do $(MKDIR) `$(DIRNAME) "include/$(MODULEPATH)/$${i}"` && $(INSTALL) -p -m 0644 "$${i}" "include/$(MODULEPATH)/$${i}"; done
endif

ifneq ($(strip $(ReleaseAsDebug)),1)

$(EXECUTABLE): $(OBJS_REL) $(STATICS_DEPS_REL)
	@$(ECHO_LDR) $@
	@$(MKDIR) $(dir $(EXECUTABLE))
	@$(CXX) $(OBJS_REL) $(STATICS_REL) $(LDFLAGS_LIB) $(STATICS_REL) $(LDFLAGS_LIB) $(LDFLAGS_REL_FINAL) -o $@
	@$(OBJCOPY) --only-keep-debug $(EXECUTABLE) $(EXECUTABLE).syms 2>/dev/null; \
     if [ $$? -ne 0 ]; then $(TRUE); else \
     $(CHMOD) a-x $(EXECUTABLE).syms && \
     $(OBJCOPY) --strip-unneeded $(EXECUTABLE) && \
     $(CD) $(dir $(EXECUTABLE)) && $(OBJCOPY) --add-gnu-debuglink=$(notdir $(EXECUTABLE)).syms $(notdir $(EXECUTABLE)); fi

$(LIBRARY): $(OBJS_REL)
	@$(ECHO_ARR) $@
	@$(MKDIR) $(dir $(LIBRARY))
	@$(AR) rcs $@ $(OBJS_REL)

$(SHLIBRARY): $(OBJS_REL) $(STATICS_DEPS_REL)
	@$(ECHO_LDR) $@
	@$(MKDIR) $(dir $(SHLIBRARY))
	@$(CXX) $(OBJS_REL) $(STATICS_REL) $(LDFLAGS_LIB) $(STATICS_REL) $(LDFLAGS_LIB) $(LDFLAGS_REL_FINAL) -shared -Wl,--soname=$(SHLIBRARYNAME) -o $(SHLIBRARYREAL)
	@$(LN) -s $(SHLIBRARYREALNAME) $@

define rule_cgi_release
$(1): $(patsubst $(TARGET)/%,%,$(dir $(1))).libs/$(patsubst %$(CGI_SUFFIX),%_cgi.cpp.release.o,$(notdir $(1))) $(CGI_OBJ_REL) $(STATICS_DEPS_REL)
	@$$(ECHO_LDR) $$@
	@$$(MKDIR) $$(dir $$@)
	@$$(CXX) $$< $$(CGI_OBJ_REL) \
			$$(call make_release_objects,$$($$(call nolibs,$$@)_SOURCES)) \
			$$(STATICS_REL) $$(LDFLAGS_LIB) $$(STATICS_REL) $$(LDFLAGS_LIB) $$(LDFLAGS_REL_FINAL) -o $$@
endef

define rule_test_release
$(1): $(patsubst $(TARGET)/%,%,$(dir $(1))).libs/$(patsubst %,%.cpp.release.o,$(notdir $(1))) $(TEST_OBJ_REL) $(STATICS_DEPS_REL)
	@$$(ECHO_LDR) $$@
	@$$(MKDIR) $$(dir $$@)
	@$$(CXX) $$< $$(TEST_OBJ_REL) \
			$$(call make_release_objects,$$($$(call nolibs,$$@)_SOURCES)) \
			$$(STATICS_REL) $$(LDFLAGS_LIB) $$(STATICS_REL) $$(LDFLAGS_LIB) $$(LDFLAGS_REL_FINAL) -o $$@
endef

%.pb.cc.release.o: $(dir %)../$(notdir %.pb.cc)
	@$(ECHO_CCR) $(call clean,$<)
	@$(CXX) $(CXXFLAGS_REL_FINAL) -I$(dir $@) -c -o $@ $(call clean,$<)

%.thrift.release.o: $(dir %)../$(notdir %.cpp)
	@$(ECHO_CCR) $(call clean,$<)
	@$(CXX) $(CXXFLAGS_REL_FINAL) -c -o $@ $<

%.cpp.release.o: $(dir %)../$(notdir %.cpp)
	@$(ECHO_CCR) $(call clean,$<)
	@$(CXX) $(CXXFLAGS_REL_FINAL) -c -o $@ $(call clean,$<)

%.c.release.o: $(dir %)../$(notdir %.c)
	@$(ECHO_CCR) $(call clean,$<)
	@$(CC) $(CFLAGS_REL_FINAL) -c -o $@ $(call clean,$<)

%.cpp.release.deps: $(dir %)../$(notdir %.cpp)
	@$(CXX) $(CXXFLAGS_REL_FINAL) -MT '$(basename $@).o $@' -E -M -MF $@ $< >/dev/null 2>&1 && \
        $(ECHO) "$(basename $@).o $@: $(call make_release_objects,$($(basename $@)_SOURCES))" >>$@ || ($(RM) $@; $(RM) $(basename $@).o)

%.c.release.deps: $(dir %)../$(notdir %.c)
	@$(CC) $(CFLAGS_REL_FINAL) -MT '$(basename $@).o $@' -E -M -MF $@ $< >/dev/null 2>&1 && \
        $(ECHO) "$(basename $@).o $@: $(call make_release_objects,$($(basename $@)_SOURCES))" >>$@ || ($(RM) $@; $(RM) $(basename $@).o)

else # ReleaseAsDebug

define rule_cgi_release
$(1): $(patsubst %$(CGI_SUFFIX),%_debug$(CGI_SUFFIX),$(1))
	@$$(LN) $$< $$@
endef
define rule_test_release
$(1): $(patsubst %,%_debug,$(1))
	@$$(LN) $$< $$@
endef

$(EXECUTABLE): $(EXECUTABLEd)
	@$(LN) $< $@
$(LIBRARY): $(LIBRARYd)
	@$(LN) $< $@
$(SHLIBRARY): $(SHLIBRARYd)
	@$(LN) $< $(SHLIBRARYREAL)
	@$(LN) -s $(SHLIBRARYREALNAME) $@
%.pb.cc.release.o:  %.pb.cc.debug.o
	@$(LN) -s $(notdir $<) $@
%.thrift.release.o: %.thrift.debug.o
	@$(LN) -s $(notdir $<) $@
%.cpp.release.o:    %.cpp.debug.o
	@$(LN) -s $(notdir $<) $@
%.c.release.o:      %.c.debug.o
	@$(LN) -s $(notdir $<) $@
%.cpp.release.deps: %.cpp.debug.deps
	@$(LN) -s $(notdir $<) $@
%.c.release.deps:   %.c.debug.deps
	@$(LN) -s $(notdir $<) $@

endif # ReleaseAsDebug

$(EXECUTABLEd): $(OBJS_DBG) $(STATICS_DEPS_DBG)
	@$(ECHO_LDD) $@
	@$(MKDIR) $(dir $(EXECUTABLEd))
	@$(CXX) $(OBJS_DBG) $(STATICS_DBG) $(LDFLAGS_LIB) $(STATICS_DBG) $(LDFLAGS_LIB) $(LDFLAGS_DBG_FINAL) -o $@

$(LIBRARYd): $(OBJS_DBG)
	@$(ECHO_ARD) $@
	@$(MKDIR) $(dir $(LIBRARYd))
	@$(AR) rcs $@ $(OBJS_DBG)

$(SHLIBRARYd): $(OBJS_DBG) $(STATICS_DEPS_DBG)
	@$(ECHO_LDD) $@
	@$(MKDIR) $(dir $(SHLIBRARY))
	@$(CXX) $(OBJS_DBG) $(STATICS_DBG) $(LDFLAGS_LIB) $(STATICS_DBG) $(LDFLAGS_LIB) $(LDFLAGS_DBG_FINAL) -shared -Wl,--soname=$(SHLIBRARYNAME) -o $@

define rule_cgi_debug
$(1): $(patsubst $(TARGET)/%,%,$(dir $(1))).libs/$(patsubst %_debug$(CGI_SUFFIX),%_cgi.cpp.debug.o,$(notdir $(1))) $(CGI_OBJ_DBG) $(STATICS_DEPS_DBG)
	@$$(ECHO_LDD) $$@
	@$$(MKDIR) $$(dir $$@)
	@$$(CXX) $$< $$(CGI_OBJ_DBG) \
			$$(call make_debug_objects,$$($$(call nolibs,$$@)_SOURCES)) \
			$$(STATICS_DBG) $$(LDFLAGS_LIB) $$(STATICS_DBG) $$(LDFLAGS_LIB) -Wl,--rpath=../lib $$(LDFLAGS_DBG_FINAL) -o $$@
endef

define rule_test_debug
$(1): $(patsubst $(TARGET)/%,%,$(dir $(1))).libs/$(patsubst %_debug,%.cpp.debug.o,$(notdir $(1))) $(TEST_OBJ_DBG) $(STATICS_DEPS_DBG)
	@$$(RM) $(patsubst $(TARGET)/%,%,$(dir $(1)))$(notdir $(1))
	@$$(ECHO_LDD) $$@
	@$$(MKDIR) $$(dir $$@)
	@$$(CXX) $$< $$(TEST_OBJ_DBG) \
			$$(call make_debug_objects,$$($$(call nolibs,$$@)_SOURCES)) \
			$$(STATICS_DBG) $$(LDFLAGS_LIB) $$(STATICS_DBG) $$(LDFLAGS_LIB) -Wl,--rpath=../lib $$(LDFLAGS_DBG_FINAL) -o $$@
endef

$(foreach cgi,$(CGI),$(eval $(call rule_cgi_release,$(cgi))))
$(foreach cgi,$(CGId),$(eval $(call rule_cgi_debug,$(cgi))))
$(foreach test,$(TEST),$(eval $(call rule_test_release,$(test))))
$(foreach test,$(TESTd),$(eval $(call rule_test_debug,$(test))))

%.pb.cc.debug.o: $(dir %)../$(notdir %.pb.cc)
	@$(ECHO_CCD) $(call clean,$<)
	@$(CXX) $(CXXFLAGS_DBG_FINAL) -I$(dir $@) -c -o $@ $(call clean,$<)

%.thrift.debug.o: $(dir %)../$(notdir %.cpp)
	@$(ECHO_CCD) $(call clean,$<)
	@$(CXX) $(CXXFLAGS_DBG_FINAL) -c -o $@ $<

%.cpp.debug.o: $(dir %)../$(notdir %.cpp)
	@$(ECHO_CCD) $(call clean,$<)
	@$(CXX) $(CXXFLAGS_DBG_FINAL) -c -o $@ $(call clean,$<)

%.c.debug.o: $(dir %)../$(notdir %.c)
	@$(ECHO_CCD) $(call clean,$<)
	@$(CC) $(CFLAGS_DBG_FINAL) -c -o $@ $(call clean,$<)

%.pb.cc %.pb.h: %.proto
	@$(ECHO_PBC) $(call clean,$<)
	@$(PROTOC) --cpp_out=$(call cleandir,$<) --proto_path=$(call cleandir,$<) $(call clean,$<)

%_constants.cpp %_types.cpp: $(dir %)../$(notdir %.thrift)
	@$(ECHO_TFT) $(call cleantft,$<)
	@$(THRIFT) --gen cpp -strict --out $(dir $@) $<

%.cpp.debug.deps: $(dir %)../$(notdir %.cpp)
	@$(CXX) $(CXXFLAGS_DBG_FINAL) -MT '$(basename $@).o $@' -E -M -MF $@ $< >/dev/null 2>&1 && \
        $(ECHO) "$(basename $@).o $@: $(call make_debug_objects,$($(basename $@)_SOURCES))" >>$@ || ($(RM) $@; $(RM) $(basename $@).o)

%.c.debug.deps: $(dir %)../$(notdir %.c)
	@$(CC) $(CFLAGS_DBG_FINAL) -MT '$(basename $@).o $@' -E -M -MF $@ $< >/dev/null 2>&1 && \
        $(ECHO) "$(basename $@).o $@: $(call make_debug_objects,$($(basename $@)_SOURCES))" >>$@ || ($(RM) $@; $(RM) $(basename $@).o)

# Same as .PRECIOUS, remember to synchronize.
# HACK(yiyuanzhong): make 3.81 misunderstands '.libs/Makefile.dep: .PRECIOUS'.
.libs/Makefile.dep: $(foreach proto,$(PROTOS),$(dir $(proto)).libs/../$(notdir $(basename $(proto))).pb.h) \
                    $(foreach proto,$(PROTOS),$(dir $(proto)).libs/../$(notdir $(basename $(proto))).pb.cc) \
                    $(foreach thrift,$(THRIFTS),$(dir $(thrift))gen-cpp/$(notdir $(basename $(thrift)))_types.cpp) \
                    $(foreach thrift,$(THRIFTS),$(dir $(thrift))gen-cpp/$(notdir $(basename $(thrift)))_constants.cpp)
	@$(TOUCH) $@

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),distclean)
ifneq ($(MAKECMDGOALS),debug-clean)
ifneq ($(MAKECMDGOALS),release-clean)
ifneq ($(strip $(DEPS_REL) $(DEPS_DBG)),)
-include $(DEPS_REL) $(DEPS_DBG)
endif
ifneq ($(strip $(THRIFTS)),)
-include .libs/Makefile.dep
endif
endif
endif
endif
endif
