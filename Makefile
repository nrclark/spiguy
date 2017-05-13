SRC := spiguy.c
NAME := spiguy
SYNTAX_OPTS := -Wall -Wextra -pedantic -Werror -fsyntax-only

CC := gcc
CFLAGS := -O2

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
