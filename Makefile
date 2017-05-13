SRC := spiguy.c
NAME := spiguy
SYNTAX_OPTS := -Wall -Wextra -pedantic -Werror -fsyntax-only

CC := gcc
CFLAGS := -O2

prefix ?= /usr
bindir ?= $(prefix)/bin
DESTDIR ?= 

all: $(NAME)

define \n


endef

$(NAME):

syntax:
	$(foreach x,$(SRC),$(CC) $(SYNTAX_OPTS)$(\n))

format:
	$(foreach x,$(SRC),astyle --options=astyle.opts $(x)$(\n))

$(NAME): $(SRC)
	$(CC) $(CFLAGS)	$(filter %.c,$^) -o $@

clean:
	rm -f $(NAME)

install: $(NAME)
	cp $(NAME) $(abspath $(DESTDIR)/$(bindir))
	chmod 755 $(abspath $(DESTDIR)/$(bindir)/$(NAME))
	chown root:root $(abspath $(DESTDIR)/$(bindir)/$(NAME))
