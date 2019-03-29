# define the C compiler to use
CC = gcc

# define any compile-time flags
CFLAGS = -std=c99 -Wall -Wextra -pedantic -Wmissing-prototypes -Wshadow -O3 -flto -fomit-frame-pointer

# define any directories containing header files other than /usr/include
IDIR = include
DEPS = $(wildcard $(IDIR)/*.h)

# define any libraries
LDIR = lib

# add includes
CFLAGS += -I$(IDIR) -I$(LDIR)/hash_table_c/include/ -I$(LDIR)/jsmn/

# define the C source files
SDIR = src
SRCS = $(wildcard $(SDIR)/*.c)

# Object files
ODIR = obj
OBJS = $(patsubst %,$(ODIR)/%,$(notdir $(SRCS:.c=.o)))

#
# The following part of the makefile is generic; it can be used to 
# build any executable just by changing the definitions above and by
# deleting dependencies appended to the file from 'make depend'
#

default: json_parser

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

json_parser: $(OBJS)
	$(CC) -o $@ $(LDIR)/hash_table_c/obj/hash_table.o $(LDIR)/jsmn/jsmn.o $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	$(RM) $(ODIR)/*.o json_parser *~ core $(INCDIR)/*~
