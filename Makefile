# List of source files that are part of the kernel (can be .c, .s, or .S files).
SOURCES = $(wildcard *.c) $(wildcard *.s) $(wildcard *.S)

# List of header files used by the sources (all files *.h in current directory).
HEADERS = $(wildcard *.h)

# List of object files that are part of the kernel (derived from sources).
OBJECTS = $(patsubst %.S,obj/%.o,$(patsubst %.s,obj/%.o,$(patsubst %.c,obj/%.o,$(SOURCES))))

# Path to the compiler.
CCTMP := $(shell which mipsel-linux-gnu-gcc 2> /dev/null)
ifdef CCTMP
	CC := mipsel-linux-gnu-gcc
else
	#CC := /courses/cs3410/mipsel-linux/bin/mipsel-linux-gcc
	CC := mipsel-linux-gcc
endif

# Turn on all warnings.
COMMONFLAGS += -Wall 

# Make all warnings into fatal errors.
COMMONFLAGS += -Werror 

# Optionally turn on compiler optimizations. 
#
# BEWARE: This can cause your code to do very unexpected things, such as
# optimize away entire loops if the compiler doesn't think they are important.
# It also can make debugging very tricky, as the compiler will re-order
# statements as it sees fit.  You will almost certainly need to examine the
# assembly code produced under optimizations, especially wherever you use inline assembly. 
#COMMONFLAGS += -O3


#
# You probably will not need to modify anything below here
#

# Tell compiler to disable normal system calls, dynamic libraries, indirect jumping, etc.
COMMONFLAGS += -static -mno-xgot -mno-abicalls -G 0 -Wa,-non_shared 

# Tell compiler to use a more recent version of mips so we can use LL and SC.
# It is probably safer to do this on a file by file basis, using ".set mips2"
# or ".set mips3", etc., in the assembly, as needed.
#COMMONFLAGS += -mips2

# Tell compiler to add debugging symbols to compiled objects, so that
# objdump -S shows more interesting stuff.
COMMONFLAGS += -g3

# Tell compiler to accept C99 extensions, such as "for (int i = ...)", along
# with some gnu-specific gcc language extensions.
COMMONFLAGS += -std=gnu99

# Link options: Tell linker to disable normal system libraries, and use
# kernel's unusual program layout.
LINKFLAGS = $(FLAGS) -nostartfiles -nodefaultlibs -Wl,-T,kernel.x

# Merge the common flags and the per-file FLAGS_xxx settings.
FLAGS = $(COMMONFLAGS) $(FLAGS_$(basename $(notdir $@)))

# 'make' or 'make all' or 'make kernel' compiles the kernel
all: kernel

kernel: $(OBJECTS) kernel.x Makefile $(HEADERS)
	$(CC) $(LINKFLAGS) -o $@ $(OBJECTS)

# 'make <name>.o' just compiles and assembles one file, but does not link
obj/%.o: %.c $(HEADERS) Makefile
	$(CC) $(FLAGS) -c $< -o $@

obj/%.o: %.s $(HEADERS) Makefile
	$(CC) $(FLAGS) -c $< -o $@

obj/%.o: %.S $(HEADERS) Makefile
	$(CC) $(FLAGS) -c $< -o $@

# 'make <name>.s' just compiles that file, but does not assemble or link
#%.s: %.c $(HEADERS) Makefile
#	$(CC) $(FLAGS) -S $<

# 'make clean' deletes everything that has been compiled
clean:
	rm -f obj/*.o kernel
