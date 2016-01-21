# 
#

BINDIR = ../bin
SRCDIR = ../src

CFLAGS = -O -DLINUX -m32 -fPIC
CC = gcc
LIBFLAG = -shared -Wl,-rpath,.:$(BROWSERDIR)/math/lib:$(vnmrsystem)/lib \
                -L. -L$(BROWSERDIR)/math/lib -L$(vnmrsystem)/lib \
                -lddl -lport3 -lf2c -lm

#################################
# List of "simple" source files
#################################
USRSRC = addms.c maxof.c stats.c baseline.c circ.c mean.c object.c snr.c snrme.c subms.c
#################################

USREXEC = $(USRSRC:.c=)

FITSRC = fit.c \
	chisq.c \
	linfit.c \
	parmlist.c \
	ufuncs.c
FITOBJ = $(FITSRC:.c=.o)

# All source / object files
CSRC = $(FITSRC) $(USRSRC)
COBJ = $(CSRC:.c=.o)

all:
	make fit
	make $(USREXEC)
	make clean

fit: $(FITOBJ)
	$(CC) -shared -o fit -m32 -fPIC -Wl,-rpath,.:$(BROWSERDIR)/math/lib:$(vnmrsystem)/lib $(FITOBJ) \
		-L. -L$(BROWSERDIR)/math/lib -L$(vnmrsystem)/lib \
		-lddl -lport3 -lf2c -lm
	rm -f $(BINDIR)/fit
	ln -s $(SRCDIR)/fit $(BINDIR)/fit

maxof: ufuncs.o parmlist.o maxof.o
	$(CC) -o maxof -m32 -fPIC ufuncs.o parmlist.o maxof.o $(LIBFLAG)
	rm -f $(BINDIR)/maxof
	ln -s $(SRCDIR)/maxof $(BINDIR)/maxof

addms: ufuncs.o parmlist.o addms.o
	$(CC) -o addms -m32 -fPIC ufuncs.o parmlist.o addms.o $(LIBFLAG)
	rm -f $(BINDIR)/addms
	ln -s $(SRCDIR)/addms $(BINDIR)/addms

subms: ufuncs.o parmlist.o subms.o
	$(CC) -o subms -m32 -fPIC ufuncs.o parmlist.o subms.o $(LIBFLAG)
	rm -f $(BINDIR)/subms
	ln -s $(SRCDIR)/subms $(BINDIR)/subms

stats: ufuncs.o parmlist.o stats.o
	$(CC) -o stats -m32 -fPIC ufuncs.o parmlist.o stats.o $(LIBFLAG)
	rm -f $(BINDIR)/stats
	ln -s $(SRCDIR)/stats $(BINDIR)/stats

baseline: ufuncs.o parmlist.o baseline.o
	$(CC) -o baseline -m32 -fPIC ufuncs.o parmlist.o baseline.o $(LIBFLAG)
	rm -f $(BINDIR)/baseline
	ln -s $(SRCDIR)/baseline $(BINDIR)/baseline

circ: ufuncs.o parmlist.o circ.o
	$(CC) -o circ -m32 -fPIC ufuncs.o parmlist.o circ.o $(LIBFLAG)
	rm -f $(BINDIR)/circ
	ln -s $(SRCDIR)/circ $(BINDIR)/circ

mean: ufuncs.o parmlist.o mean.o
	$(CC) -o mean -m32 -fPIC ufuncs.o parmlist.o mean.o $(LIBFLAG)
	rm -f $(BINDIR)/mean
	ln -s $(SRCDIR)/mean $(BINDIR)/mean

object: ufuncs.o parmlist.o object.o
	$(CC) -o object -m32 -fPIC ufuncs.o parmlist.o object.o $(LIBFLAG)
	rm -f $(BINDIR)/object
	ln -s $(SRCDIR)/object $(BINDIR)/object

snr: ufuncs.o parmlist.o snr.o
	$(CC) -o snr -m32 -fPIC ufuncs.o parmlist.o snr.o $(LIBFLAG)
	rm -f $(BINDIR)/snr
	ln -s $(SRCDIR)/snr $(BINDIR)/snr

snrme: ufuncs.o parmlist.o snrme.o
	$(CC) -o snrme -m32 -fPIC ufuncs.o parmlist.o snrme.o $(LIBFLAG)
	rm -f $(BINDIR)/snrme
	ln -s $(SRCDIR)/snrme $(BINDIR)/snrme

clean:
	rm *.o

$(COBJ) : %.o:%.c
	$(COMPILE.c) $<

# "-Y": do not look in usual places for system include files.
# (Produces a bunch of spurious warnings about not being able
# to find header files.)
depend:	$(CSRC)
	makedepend -f makemathfunc.lnx -Y $(CSRC)

# DO NOT DELETE THIS LINE -- make depend depends on it.

fit.o: imagemath.h parmlist.h ddl_c_interface.h userfit.c t1fit.c t2fit.c
fit.o: adcfit.c qt1fit.c shamesfit.c polyfit.c
chisq.o: imagemath.h parmlist.h ddl_c_interface.h
linfit.o: f2c.h
parmlist.o: parmlist.h
ufuncs.o: imagemath.h parmlist.h ddl_c_interface.h
addms.o: imagemath.h parmlist.h ddl_c_interface.h
maxof.o: imagemath.h parmlist.h ddl_c_interface.h
stats.o: imagemath.h parmlist.h ddl_c_interface.h image_utils.h
baseline.o: imagemath.h parmlist.h ddl_c_interface.h
circ.o: imagemath.h parmlist.h ddl_c_interface.h image_utils.h
mean.o: imagemath.h parmlist.h ddl_c_interface.h
object.o: imagemath.h parmlist.h ddl_c_interface.h image_utils.h
snr.o: imagemath.h parmlist.h ddl_c_interface.h image_utils.h
snrme.o: imagemath.h parmlist.h ddl_c_interface.h image_utils.h
subms.o: imagemath.h parmlist.h ddl_c_interface.h
