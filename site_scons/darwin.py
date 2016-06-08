from SCons.Script import *
import sys
import os



def darwincompile(env):
  print "darwincompile"
  platform = sys.platform
  "sets up environment for darwin, reading bash variables"
  if (platform == 'darwin'):
    # Set this to the SDK. XCode 7 needs MacOSX10.11.sdk, Xcode 6.x needs MacOSX10.10.sdk
    # export OSX_SDK='/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk'
    if 'OSX_SDK' in os.environ:
      OSX_SDK = os.environ['OSX_SDK']
    else:
      OSX_SDK = '/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk'

    # Choose your C compiler: cc is clang, gcc is /vnmr/gcc/bin/gcc (gcc 4.9.3)
    # export CC=cc
    if 'CC' in os.environ:
      OSX_CC = os.environ['CC'];
    else:
      OSX_CC = 'gcc'

    # Choose your FORTRAN compiler: g77 or gfortran
    # export F77=gfortran
    if 'F77' in os.environ:
      OSX_F77 = os.environ['F77'];
    else:
      OSX_F77 = 'gfortran'

    # set a path to the compiler; this will be prepended to the PATH. For example, /vnmr/gcc/bin or /usr/local/bin
    # export OSX_GCC_PATH=/vnmr/gcc/bin
    if 'OSX_GCC_PATH' in os.environ:
      OSX_GCC_PATH = os.environ['OSX_GCC_PATH']
    else:
      OSX_GCC_PATH = '/vnmr/gcc/bin'

    env.Replace(CC = OSX_CC)
    env.Replace(FORTRAN = OSX_F77)
  
    if (OSX_GCC_PATH):
      env.PrependENVPath('PATH', OSX_GCC_PATH)

    if (OSX_SDK and (OSX_CC == 'gcc')):
      env.Append(CPPDEFINES = 'MACOS', CPPFLAGS = ' -arch i386 -isysroot '+OSX_SDK,
        LINKFLAGS = ' -arch i386 -L/usr/lib', FORTRANFLAGS = ' -m32 ')
    else:
      env.Append(CPPDEFINES = 'MACOS', CPPFLAGS = ' -arch i386 ',
        LINKFLAGS = ' -arch i386 -L/usr/lib ', FORTRANFLAGS = ' -m32 ')
    return
