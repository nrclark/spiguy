SRC := spiguy.c spiguy_lib.h spiguy_lib.c
NAME := spiguy

CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -pedantic -std=gnu99
SYNTAX_OPTS := $(CFLAGS) -Werror -fsyntax-only

prefix ?= /usr
bindir ?= $(prefix)/bin
DESTDIR ?= 

WARNINGS_BLACKLIST := -Wsystem-headers -Wtraditional -Wunused-macros
WARNINGS_BLACKLIST += -Wdate-time

all: $(NAME) spidev_test

.INTERMEDIATE: warn.flags
.PHONY: warn.flags
warn.flags:
	gcc $(CFLAGS) -Q --help=warning | grep -i disabled] | grep -oP "[-]W[^ ]+" > $@
	printf "" > _flags_dummy.c
	cat $@ | xargs -n1024 gcc $(CFLAGS) -c _flags_dummy.c 2>&1 | \
	    grep "not for C" | grep -Po "[-]W[a-zA-Z0-9+_-]+" > flags.blacklist
	echo $(WARNINGS_BLACKLIST) | tr ' ' '\n' | sort | uniq >> flags.blacklist
	grep -v -x -f flags.blacklist $@ | sort | uniq > flags.valid
	mv flags.valid $@
	rm -f flags.blacklist _flags_dummy.*

define \n


endef

syntax: warn.flags
	$(foreach x,$(SRC), $(CC) $(SYNTAX_OPTS) $(x)$(\n))
	-$(foreach x,$(SRC), $(CC) $(SYNTAX_OPTS) $$(cat warn.flags) $(x)$(\n))

format:
	$(foreach x,$(SRC),astyle --options=astyle.opts $(x)$(\n))

$(NAME): $(SRC)
	$(CC) $(CFLAGS) $(filter %.c,$^) -o $@

spidev_test: spidev_test.c
	$(CC) $(CFLAGS) $(filter %.c,$^) -o $@

clean:
	rm -f $(NAME) spidev_test

install: $(NAME) spidev_test
	mkdir -p $(abspath $(DESTDIR)/$(bindir))
	cp $^ $(abspath $(DESTDIR)/$(bindir))
	chmod 755 $(foreach x,$^,$(abspath $(DESTDIR)/$(bindir)/$(x)))
	chown $(shell id -un):$(shell id -gn) $(foreach x,$^,$(abspath $(DESTDIR)/$(bindir)/$(x)))
