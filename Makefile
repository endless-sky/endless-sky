PREFIX ?= /usr/local

.PHONY: release debug profile all clean install
OPTLEVEL ?= -O3

LIBS += sdl2 libpng libjpeg gl glew openal mad
CXXFLAGS += -std=c++11 -Wall
CXXFLAGS += $(shell pkg-config $(LIBS) --cflags)
LDFLAGS += $(shell pkg-config $(LIBS) --libs) -lpthread

PROG = endless-sky
PROG_DEBUG = $(PROG)-debug
PROG_PROF = $(PROG)-profile
DIR_SRC = source
DIR_BUILD = build
DIR_RELEASE = $(DIR_BUILD)/release
DIR_DEBUG = $(DIR_BUILD)/debug
DIR_PROF = $(DIR_BUILD)/profile

SRCS = $(wildcard $(DIR_SRC)/*.cpp)
OBJS = $(patsubst $(DIR_SRC)/%.cpp,$(DIR_RELEASE)/%.o,$(SRCS))
DEPS = $(patsubst $(DIR_SRC)/%.cpp,$(DIR_RELEASE)/%.d,$(SRCS))
OBJS_DEBUG = $(patsubst $(DIR_SRC)/%.cpp,$(DIR_DEBUG)/%.o,$(SRCS))
DEPS_DEBUG = $(patsubst $(DIR_SRC)/%.cpp,$(DIR_DEBUG)/%.d,$(SRCS))
OBJS_PROF = $(patsubst $(DIR_SRC)/%.cpp,$(DIR_PROF)/%.o,$(SRCS))
DEPS_PROF = $(patsubst $(DIR_SRC)/%.cpp,$(DIR_PROF)/%.d,$(SRCS))

$(info mkdir -p $(DIR_RELEASE) $(DIR_DEBUG) $(DIR_PROF))
$(shell mkdir -p $(DIR_RELEASE) $(DIR_DEBUG) $(DIR_PROF))

$(DIR_RELEASE)/%.o: $(DIR_SRC)/%.cpp
	$(CXX) $(OPTLEVEL) $(CXXFLAGS) -o $@ -c -MMD $<

$(DIR_RELEASE)/$(PROG): $(OBJS)
	$(CXX) $(OPTLEVEL) $(CXXFLAGS) -o $(DIR_RELEASE)/$(PROG) $^ $(LDFLAGS)

release: $(DIR_RELEASE)/$(PROG)
	cp $(DIR_RELEASE)/$(PROG) .

$(DIR_DEBUG)/%.o: $(DIR_SRC)/%.cpp
	$(CXX) $(CXXFLAGS) -g -o $@ -c -MMD $<

$(DIR_DEBUG)/$(PROG_DEBUG): $(OBJS_DEBUG)
	$(CXX) $(CXXFLAGS) -g -o $(DIR_DEBUG)/$(PROG_DEBUG) $^ $(LDFLAGS)

debug: $(DIR_DEBUG)/$(PROG_DEBUG)
	cp $(DIR_DEBUG)/$(PROG_DEBUG) .

$(DIR_PROF)/%.o: $(DIR_SRC)/%.cpp
	$(CXX) $(CXXFLAGS) -pg -o $@ -c -MMD $<

$(DIR_PROF)/$(PROG_PROF): $(OBJS_PROF)
	$(CXX) $(CXXFLAGS) -pg -o $(DIR_PROF)/$(PROG_PROF) $^ $(LDFLAGS)

profile: $(DIR_PROF)/$(PROG_PROF)
	cp $(DIR_PROF)/$(PROG_PROF) .

.DEFAULT: all
all: release

DESTINATION = $(DESTDIR)$(PREFIX)
PREFIX_DATA = $(DESTINATION)/share/games/$(PROG)
PREFIX_BIN = $(DESTINATION)/bin
ICON = icons/icon_
ICONS = 16x16 22x22 24x24 32x32 48x48 256x256

install: release
	mkdir -p $(PREFIX_BIN)
	install -m 0755 $(PROG) $(PREFIX_BIN)
	mkdir -p $(PREFIX_DATA)
	cp -R --no-preserve=ownership data $(PREFIX_DATA)
	cp -R --no-preserve=ownership images $(PREFIX_DATA)
	cp -R --no-preserve=ownership sounds $(PREFIX_DATA)
	cp --no-preserve=ownership credits.txt $(PREFIX_DATA)
	cp --no-preserve=ownership keys.txt $(PREFIX_DATA)
	mkdir -p $(DESTINATION)/share/man/man6/
	gzip -c $(PROG).6 > $(DESTINATION)/share/man/man6/$(PROG).6.gz
	for icon in $(ICONS); do \
		mkdir -p $(DESTINATION)/share/icons/hicolor/$$icon/apps; \
		cp --no-preserve=ownership $(ICON)$$icon.png $(DESTINATION)/share/icons/hicolor/$$icon/apps/$(PROG).png; \
	done

clean:
	$(RM) $(PROG) $(PROG_DEBUG) $(PROG_PROF)
	$(RM) -r $(DIR_RELEASE) $(DIR_DEBUG) $(DIR_PROF)

-include $(DEPS)
-include $(DEPS_DEBUG)
-include $(DEPS_PROF)
