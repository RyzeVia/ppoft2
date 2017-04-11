TARGET  = libxpt.a

ifndef PFIXENV
export XPTI_ARCH  = ftb
export CPIO_ARCH  = syscall
export TMP_INC    = include
export TMP_LIB    = lib
export FTB_HOME   = /home/jitumoto/opt/ftb
export CFR_HOME   = ../../config_reader
export CFLAG_EXT  = -Ixpti/$(XPTI_ARCH) -I$(CFR_HOME) 
export LDFLAG_EXT = -Lxpti/$(XPTI_ARCH) -L$(CFR_HOME)
export PFIXENV    = 1

export CC         = gcc
export MPICC      = mpicc
export CFLAGS	  = -c -Wall -I$(TMP_INC) -I. $(CFLAG_EXT)
export LD         = gcc
export MPILD      = mpicc
export LDFLAG     = -g -lpthread -lm -lrt -L$(TMP_LIB) -L.  $(LDFLAG_EXT) -o
export AR		  = ar
export RANLIB	  = ranlib
export ARCFLAGS	  = crsv
export ARXFLAGS	  = xv
endif

OBJS	= XPT_base.o
HEADS   = libxpt.h

#Define Inference Rules

.c.o:
	$(CC) -c $(CFLAGS) -I$(FTB_HOME)/include $*.c
	
all: $(TMP_INC) $(TMP_LIB) $(TARGET)
	cp $(HEADS) $(TMP_INC)
	cp $(TARGET) $(TMP_LIB)

$(TMP_INC):
	mkdir $(TMP_INC)
	
$(TMP_LIB):
	mkdir $(TMP_LIB)


libxpt.a: $(CFR_HOME)/libcfr.a xpti/$(XPTI_ARCH)/libxpti.a $(OBJS)  
	mkdir depos
	cd depos; $(AR) x ../xpti/$(XPTI_ARCH)/libxpti.a
	$(AR) $(ARFLAGS) $@ $(OBJS) depos/*.o
	rm -rf depos
	
xpti/$(XPTI_ARCH)/libxpti.a:
	cd xpti/$(XPTI_ARCH); $(MAKE)

$(CFR_HOME)/libcfr.a:
	cd $(CFR_HOME); $(MAKE)

clean:
	rm -f $(TARGET) $(OBJS) *.o *~
	rm -rf depos
	for hd in $(HEADS); do rm -f $(TMP_INC)/$$hd; done
	for hd in $(TARGET); do rm -f $(TMP_LIB)/$$hd; done

allclean:
	rm -f $(TARGET) $(OBJS) *.o *~
	rm -rf depos
	for hd in $(HEADS); do rm -f $(TMP_INC)/$$hd; done
	for hd in $(TARGET); do rm -f $(TMP_LIB)/$$hd; done
	cd xpti/$(XPTI_ARCH); make clean;
