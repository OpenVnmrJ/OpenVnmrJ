################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../ConvertFdfFieldToTag.o \
../FdfToDcm.o \
../ProcessFdfHeader.o \
../ProcessProcpar.o \
../ProcessTagfile.o 

CPP_SRCS += \
../ConvertFdfFieldToTag.cpp \
../FdfToDcm.cpp \
../ProcessFdfHeader.cpp \
../ProcessProcpar.cpp \
../ProcessTagfile.cpp 

OBJS += \
./ConvertFdfFieldToTag.o \
./FdfToDcm.o \
./ProcessFdfHeader.o \
./ProcessProcpar.o \
./ProcessTagfile.o 

CPP_DEPS += \
./ConvertFdfFieldToTag.d \
./FdfToDcm.d \
./ProcessFdfHeader.d \
./ProcessProcpar.d \
./ProcessTagfile.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -DDEFAULTUIDROOT=1.3.6.1.4.1.670589.1 -DDEFAULTINSTANCECREATORUID=1.3.6.1.4.1.670589.3 -DDEFAULTIMPLEMENTATIONVERSIONNAME=DCTOOL100 -DDEFAULTSOURCEAPPLICATIONENTITYTITLE=VARIAN1 -DUSEGLIBCTIMEZONE=1 -DUSEDUMBINFINITYANDNAN=1 -DHASSTDNAMESPACEFORSTREAMPOS=1 -DHASGETHOSTID=1 -DHASGETHOSTIDPROTOTYPE=1 -DUSEXMITSHMEXTENSION=1 -DDEFAULTIMPLEMENTATIONCLASSUID=1.3.6.1.4.1.670589.2 -I/home/maydesl/Varian/dicom3tools/libsrc/include/generic -I/home/maydesl/Varian/dicom3tools/libsrc/include/locale -I/home/maydesl/Varian/dicom3tools/libsrc/include/dctool -I/home/maydesl/Varian/dicom3tools/libsrc/include/pixeldat -O0 -g -c -fmessage-length=0 -Wno-deprecated -Wno-write-strings -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


