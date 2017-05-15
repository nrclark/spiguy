SRC := spiguy.c
NAME := spiguy
SYNTAX_OPTS := -Wall -Wextra -pedantic -Werror -fsyntax-only

CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -pedantic

prefix ?= /usr
bindir ?= $(prefix)/bin
DESTDIR ?= 

all: $(NAME) spidev_test

define \n


endef

syntax:
	$(foreach x,$(SRC),$(CC) $(SYNTAX_OPTS) $(x)$(\n))

format:
	$(foreach x,$(SRC),astyle --options=astyle.opts $(x)$(\n))

$(NAME): $(SRC)
	$(CC) $(CFLAGS)	$(filter %.c,$^) -o $@

spidev_test: spidev_test.c
	$(CC) $(CFLAGS)	$(filter %.c,$^) -o $@

clean:
	rm -f $(NAME) spidev_test

install: $(NAME) spidev_test
	mkdir -p $(abspath $(DESTDIR)/$(bindir))
	cp $^ $(abspath $(DESTDIR)/$(bindir))
	chmod 755 $(foreach x,$^,$(abspath $(DESTDIR)/$(bindir)/$(x)))
	chown $(shell id -un):$(shell id -gn) $(foreach x,$^,$(abspath $(DESTDIR)/$(bindir)/$(x)))
