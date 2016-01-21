
# Installation Directory
BINDIR = $(HOME)/bin

LIBPATH = /usr/local/lib
INCPATH = /usr/local/include

GSL = -lgsl -lgslcblas

dOBJ  = main_diffusion.o
dEXEC = diffcalc
 
fOBJ  = main_fdffit.o 
fEXEC = fdffit
 
aOBJ  = main_asl.o 
aEXEC = asl

tOBJ  = main_T1fit.o 
tEXEC = t1fit

aeOBJ  = main_addechoes.o 
aeEXEC = addechoes

LIBES = -L$(LIBPATH) -Wl -lm $(GSL) 

all: $(dOBJ) $(fOBJ) $(aOBJ) $(tOBJ) $(aeOBJ)
	gcc -m64 $(dOBJ) $(LIBES) -o $(dEXEC)
	gcc -m64 $(fOBJ) $(LIBES) -o $(fEXEC)
	gcc -m64 $(aOBJ) $(LIBES) -o $(aEXEC)
	gcc -m64 $(tOBJ) $(LIBES) -o $(tEXEC)
	gcc -m64 $(aeOBJ) $(LIBES) -o $(aeEXEC)

diff: $(dOBJ)
	gcc $(dOBJ) $(LIBES) -o $(dEXEC)

fdffit: $(fOBJ)
	gcc -m64 $(fOBJ) $(LIBES) -o $(fEXEC)

asl: $(aOBJ)
	gcc -m64 $(aOBJ) $(LIBES) -o $(aEXEC)

t1fit: $(tOBJ)
	gcc -m64 $(tOBJ) $(LIBES) -o $(tEXEC)

addechoes: $(aOBJ)
	gcc -m64 $(aeOBJ) $(LIBES) -o $(aeEXEC)

clean:
	rm -f $(dEXEC) $(fEXEC) $(aEXEC) $(tEXEC) $(aeEXEC) *.o core
	
.c.o:
	gcc -c -pedantic -Wall -O4 -std=gnu9x -I. -I$(INCPATH) $<

install:
	if [ ! -d $(BINDIR) ]; then mkdir $(BINDIR) ; fi ; 
	if [ -f $(dEXEC) ]; then mv $(dEXEC) $(BINDIR) ; fi ;
	if [ -f $(fEXEC) ]; then mv $(fEXEC) $(BINDIR) ; fi ;
	if [ -f $(aEXEC) ]; then mv $(aEXEC) $(BINDIR) ; fi ;
	if [ -f $(tEXEC) ]; then mv $(tEXEC) $(BINDIR) ; fi ;
	if [ -f $(aeEXEC) ]; then mv $(aeEXEC) $(BINDIR) ; fi 
