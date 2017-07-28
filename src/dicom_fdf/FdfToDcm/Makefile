#
# Varian, Inc. All Rights Reserved.
# This software contains proprietary and confidential
# information of Varian, Inc. and its contributors.
# Use, disclosure and reproduction is prohibited without
# prior consent.
#

TARGET=FdfToDcm

CC = g++

INCLUDES = -I../../dicom3tools/libsrc/include/generic -I../../dicom3tools/libsrc/include/locale \
           -I../../dicom3tools/libsrc/include/dctool -I../../dicom3tools/libsrc/include/pixeldat

DEFINES = -DDEFAULTUIDROOT=\"1.3.6.1.4.1.670589.1\" \
          -DDEFAULTINSTANCECREATORUID=\"1.3.6.1.4.1.670589.3\" \
          -DDEFAULTIMPLEMENTATIONCLASSUID=\"1.3.6.1.4.1.670589.2\" \
          -DDEFAULTIMPLEMENTATIONVERSIONNAME=\"DCTOOL100\" \
          -DDEFAULTSOURCEAPPLICATIONENTITYTITLE=\"VARIAN1\" \
          -DUSEGLIBCTIMEZONE=1 -DUSEDUMBINFINITYANDNAN=1 \
          -DHASSTDNAMESPACEFORSTREAMPOS=1 -DHASGETHOSTID=1 \
          -DHASGETHOSTIDPROTOTYPE=1 -DUSEXMITSHMEXTENSION=1

CFLAGS = -Wno-deprecated -Wno-write-strings -O -g $(INCLUDES) $(DEFINES)

COMPILE = $(CC) $(CFLAGS) -c

OBJFILES := $(patsubst %.cpp,%.o,$(wildcard *.cpp))

LDFLAGS = -L../../dicom3tools/libsrc/lib -ldctl -ldlcl -lgener -lm

all: $(TARGET)

$(TARGET): $(OBJFILES)
	$(CC) -o $(TARGET) $(OBJFILES) $(LDFLAGS)

%.o: %.cpp
	$(COMPILE) -o $@ $<

clean:
	rm -f *.o $(TARGET)
