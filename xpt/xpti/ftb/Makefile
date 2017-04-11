#Define Macro

TARGET  = libxpti.a

ifndef PFIXENV
export TMP_INC    = include
export TMP_LIB    = lib
export FTB_HOME   = /home/jitumoto/opt/ftb
export CFLAG_EXT  = 
export LDFLAG_EXT =
export PFIXENV    = 1
export CC      = gcc
export LD      = gcc
export CFLAGS	  = -Wall -I$(TMP_INC) -I. $(CFLAG_EXT)
export LD         = gcc
export MPILD      = mpicc
export LDFLAG     = -g -lpthread -lm -lrt -lftb -L$(TMP_LIB) -L.  $(LDFLAG_EXT) -o
export AR		  = ar
export RANLIB	  = ranlib
export ARCFLAGS	  = crsv
export ARXFLAGS	  = xv
endif

OBJS	= xptftb.o
HEADS   = libxpti.h

#Define Inference Rules

.c.o:
	$(CC) -c $(CFLAGS) $*.c
	
all: $(TMP_INC) $(TMP_LIB) $(TARGET)
	cp $(HEADS) $(TMP_INC)
	cp $(TARGET) $(TMP_LIB)

$(TMP_INC):
	mkdir $(TMP_INC)
	
$(TMP_LIB):
	mkdir $(TMP_LIB)

	
libxpti.a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)
#	$(RANLIB) $@
	
test: toytest/xftbtest.o libxpti.a
	$(LD) $(LDFLAGS) $@ toytest/xftbtest.o libxpti.a
	if [ ! -f "tsrv" ]; then ln -s test tsrv; fi
	if [ ! -f "tcli" ]; then ln -s test tcli; fi

clean:
	rm -f $(TARGET) $(OBJS) toytest/xftbtest.o test tsrv tcli *~
	for hd in $(HEADS); do rm -f $(TMP_INC)/$$hd; done
	for hd in $(TARGET); do rm -f $(TMP_LIB)/$$hd; done
	