define nl
 
  
endef

scrub_files_call = $(foreach f,$(wildcard $(1)),$(RM) -r -- '$f'$(nl))

VERSION = 0.0.1

release_tarball = dmkbsc-$(VERSION).tar.gz

release_targets = LICENSE README.md dmkbsc dmkbsc.exe

ship_dir = ship

CPPFLAGS = -I dmklib '-DVERSION="$(VERSION)"'
CFLAGS = -Wall -Werror -Wfatal-errors
#CFLAGS = -Wall

OBJS = dmklib/libdmk.o dmkbsc.o

libdmk_args = SOURCES=libdmk.c TARGETS=libdmk.o CC=$(CC)

clean_files     = $(OBJS) dmkbsc dmkbsc.exe
clobber_files   = $(clean_files) $(ship_dir) $(release_tarball)
distclean_files = $(clobber_files)

all: dmkbsc

dmkbsc: dmkbsc.o dmklib/libdmk.o

dmkbsc.exe: CC = i586-pc-msdosdjgpp-gcc
dmkbsc.exe: dmkbsc.o dmklib/libdmk.o
	$(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -o $@

dmklib/libdmk.o: dmklib/libdmk.c dmklib/libdmk.h
	$(MAKE) -C dmklib $(libdmk_args)

release: $(release_tarball) | $(ship_dir)

$(release_tarball): $(addprefix $(ship_dir)/,$(release_targets))
	tar -cf $@ -C '$(ship_dir)' $(release_targets)

$(ship_dir)/dmkbsc $(ship_dir)/dmkbsc.exe: | $(ship_dir)
	$(MAKE) clean
	$(MAKE) '$(@F)'
	cp -f -- '$(@F)' '$(ship_dir)'

$(ship_dir):
	mkdir -p -- '$(ship_dir)'

$(ship_dir)/%: % | $(ship_dir)
	cp -f -- '$(@F)' '$(ship_dir)'

clean clobber distclean:
	$(call scrub_files_call,$($@_files))
	$(MAKE) -C dmklib $(libdmk_args) clean

.PHONY: all clean clobber distclean release
.DELETE_ON_ERROR:
