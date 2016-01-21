#
# Makefile #
#*---------------------------------------------------------------------------*#
#*                                                                           *#
#* Makefile: Makefile for aslmirtime                                         *#
#*                                                                           *#
#* This file is part of aslmirtime.                                          *#
#*                                                                           *#
#* aslmirtime is free software: you can redistribute it and/or modify        *#
#* it under the terms of the GNU General Public License as published by      *#
#* the Free Software Foundation, either version 3 of the License, or         *#
#* (at your option) any later version.                                       *#
#*                                                                           *#
#* aslmirtime is distributed in the hope that it will be useful,             *#
#* but WITHOUT ANY WARRANTY; without even the implied warranty of            *#
#* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the              *#
#* GNU General Public License for more details.                              *#
#*                                                                           *#
#* You should have received a copy of the GNU General Public License         *#
#* along with aslmirtime. If not, see <http://www.gnu.org/licenses/>.        *#
#*                                                                           *#
#*---------------------------------------------------------------------------*#
#**#

# Installation Directory
BINDIR = $(HOME)/bin
# Windoze
WBINDIR = ~/bin

# Source
SRC = $(wildcard */*.c *.c)

# Objects
OBJS  = $(SRC:.c=.o)
OBJS_DEBUG = $(SRC:.c=.debug.o)

# Includes, Library Defines
INCL  =
LIBS  = -lgsl -lgslcblas -lm

# Executable
PROG  = aslmirtime
# Windoze
WPROG = $(PROG).exe

# Compiler, Linker Defines
CC      = gcc
CFLAGS  = -pedantic -Wall -O4 -std=gnu9x
LIBPATH = -L /usr/local/lib
LDFLAGS = -o $(PROG) $(LIBPATH) $(LIBS)
DEBUG   = -pedantic -Wall -O4 -g -std=gnu9x -DDEBUG

# Compile and Assemble C Source Files into Object Files
%.o: %.c
	$(CC) -c $(CFLAGS) -o $*.o $*.c

%.debug.o: %.c
	$(CC) -c $(DEBUG) -o $*.debug.o $*.c

# Link all Object Files with external Libraries into Binaries
$(PROG): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS)

# Objects depend on these Libraries
$(OBJS): $(INCL)

# DEBUG flags turned on
debug: $(OBJS_DEBUG)
	$(CC) $(OBJS_DEBUG) $(LDFLAGS)

install:
	if [ ! -d $(BINDIR) ]; then mkdir $(BINDIR) ; fi ; mv $(PROG) $(BINDIR)

# Windoze
winstall:
	if [ ! -d $(WBINDIR) ]; then mkdir $(WBINDIR) ; fi ; if [ -f $(WPROG) ]; then mv $(WPROG) $(PROG) ; fi ; mv $(PROG) $(WBINDIR)

clean:
	rm -f $(OBJS) $(OBJS_DEBUG) $(PROG) $(WPROG) core
