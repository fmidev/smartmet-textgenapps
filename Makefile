MODULE = textgenapps
SPEC = smartmet-textgenapps

REQUIRES = gdal fmt

include $(shell echo $${PREFIX-/usr})/share/smartmet/devel/makefile.inc

# Default compiler flags

DEFINES = -DUNIX

INCLUDES += \
	-I$(includedir)/smartmet

LIBS += -L$(libdir) \
	-lsmartmet-calculator \
	-lsmartmet-textgen \
	-lsmartmet-newbase \
	-lsmartmet-macgyver \
	-L$(libdir)/mysql -lmysqlpp \
	$(REQUIRED_LIBS) \
	-lboost_iostreams \
	-lboost_locale \
	-lboost_system \
	-lstdc++ -lm

# Compilation directories

vpath %.cpp source main
vpath %.h include
vpath %.o $(objdir)
vpath %.d $(objdir)

# The files to be compiled

HDRS = $(patsubst include/%,%,$(wildcard *.h include/*.h))

MAINSRCS     = $(patsubst main/%,%,$(wildcard main/*.cpp))
MAINPROGS    = $(MAINSRCS:%.cpp=%)
MAINOBJS     = $(MAINSRCS:%.cpp=%.o)
MAINOBJFILES = $(MAINOBJS:%.o=obj/%.o)

SRCS     = $(patsubst source/%,%,$(wildcard source/*.cpp))
OBJS     = $(SRCS:%.cpp=%.o)
OBJFILES = $(OBJS:%.o=obj/%.o)

INCLUDES := -Iinclude $(INCLUDES)

# For make depend:

ALLSRCS = $(wildcard main/*.cpp source/*.cpp)

.PHONY: test rpm

# The rules

all: objdir $(MAINPROGS)
debug: objdir $(MAINPROGS)
release: objdir $(MAINPROGS)
profile: objdir $(MAINPROGS)

.SECONDEXPANSION:
$(MAINPROGS): % : obj/%.o $(OBJFILES)
	$(CXX) $(LDFLAGS) -o $@ obj/$@.o $(OBJFILES) $(LIBS)

clean:
	rm -f $(MAINPROGS) $(OBJFILES) $(MAINOBJFILES)*~ source/*~ include/*~
	rm -rf obj/

format:
	clang-format -i -style=file main/*.cpp

install:
	mkdir -p $(bindir)
	@list='$(MAINPROGS)'; \
	for prog in $$list; do \
	  echo $(INSTALL_PROG) $$prog $(bindir)/$$prog; \
	  $(INSTALL_PROG) $$prog $(bindir)/$$prog; \
	done

test:
	cd test && make test

objdir:
	@mkdir -p $(objdir)

rpm: clean $(SPEC).spec
	rm -f $(SPEC).tar.gz # Clean a possible leftover from previous attempt
	tar -czvf $(SPEC).tar.gz --exclude test --exclude-vcs --transform "s,^,$(SPEC)/," *
	rpmbuild -tb $(SPEC).tar.gz
	rm -f $(SPEC).tar.gz

.SUFFIXES: $(SUFFIXES) .cpp

obj/%.o : %.cpp
	$(CXX) $(CFLAGS) $(INCLUDES) -c -MD -MF $(patsubst obj/%.o, obj/%.d, $@) -MT $@ -o $@ $<

-include obj/*.d
