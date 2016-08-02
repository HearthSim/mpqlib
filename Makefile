CC = gcc -m32
LD = gcc -m32
AR = ar rcs

CPPFLAGS = -g -Wall -Werror -D_FILE_OFFSET_BITS=64 -I.
LDFLAGS = -g -lz

SOURCE_LIST = \
errors.c \
lookupa.c \
hashtab.c \
recycle.c \
mpq-bios.c \
mpq-crypto.c \
mpq-errors.c \
mpq-file.c \
mpq-fs.c \
mpq-misc.c \
stalloc.c \


TARGET = mpqlib.a


OBJ_LIST = $(addsuffix .o, $(notdir $(basename $(SOURCE_LIST))))
DEP_LIST = $(addsuffix .dep, $(notdir $(basename $(SOURCE_LIST))))

all: dep $(TARGET)

$(TARGET): $(OBJ_LIST)
	$(AR) $@ $(OBJ_LIST)

test-it: $(TARGET) test-it.o
	$(LD) -o test-it test-it.o $(TARGET) $(LDFLAGS)

clean:
	rm -f *.o *.dep $(TARGET) test-it

dep: $(DEP_LIST)

%.dep : %.c
	$(CC) $(CPPFLAGS) -M -MF $@ $<

-include $(DEP_LIST)
