SRC := spiguy.c
NAME := spiguy
SYNTAX_OPTS := -Wall -Wextra -pedantic -Werror -fsyntax-only

CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -pedantic

prefix ?= /usr
bindir ?= $(prefix)/bin
DESTDIR ?= 

all: $(NAME)

define \n


endef

$(NAME):

syntax:
	$(foreach x,$(SRC),$(CC) $(SYNTAX_OPTS) $(x)$(\n))

format:
	$(foreach x,$(SRC),astyle --options=astyle.opts $(x)$(\n))

$(NAME): $(SRC)
	$(CC) $(CFLAGS)	$(filter %.c,$^) -o $@

clean:
	rm -f $(NAME)

install: $(NAME)
	mkdir -p $(abspath $(DESTDIR)/$(bindir))
	cp $(NAME) $(abspath $(DESTDIR)/$(bindir))
	chmod 755 $(abspath $(DESTDIR)/$(bindir)/$(NAME))
	chown $(shell id -un):$(shell id -gn) $(abspath $(DESTDIR)/$(bindir)/$(NAME))
