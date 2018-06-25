CFILES := main.c parser.c utils.c job.c royaldutch.c
PROG := royaldutch

CC = gcc
CPP_FLAGS = -I. -Wall -Werror -std=c89 --pedantic-errors -D_POSIX_C_SOURCE=200112L 
C_FLAGS = 
LD_FLAGS = -L.
MAKE = make

OBJFILES := $(CFILES:.c=.o)
DEPFILES := $(CFILES:.c=.d)

$(PROG) : $(OBJFILES)
	$(LINK.o) $(LDFLAGS) -o $@ $^

clean :
	rm -f $(PROG) $(OBJFILES) $(DEPFILES)

-include $(DEPFILES)
