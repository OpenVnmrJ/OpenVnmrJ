#

from __future__ import print_function
import os
import sys
import string
import subprocess

ovjtools=os.getenv('OVJ_TOOLS')
if not ovjtools:
    print("OVJ_TOOLS env not found.")
    print("For bash and variants, use export OVJ_TOOLS=<path>")
    print("For csh and variants,  use setenv OVJ_TOOLS <path>")
    sys.exit(1)

if not os.path.exists(ovjtools):
    print("OVJ_TOOLS path "+ovjtools+" not found.")
    sys.exit(1)

# os.environ['OPENVNMRJ']="true"
# os.environ['OPENVNMRJ_GSL']="false"
# os.environ['OPENVNMRJ_GSL']="true"

platform = sys.platform        # sys.platform -> 'linux2' linux, 'interix6' win7 SUA
print("Platform: ", platform)


#
# top level build file
#

# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# WHEN ADDING NEW FILES TO THIS BUILD, BE SURE YOU TEST
# THE BUILD BY INVOKING:
#
#   scons -c && scons -j3
#
# All builds MUST be thread-safe so that parallel builds
# function properly.  Parallel building is a simple way
# to cut down on compilation time.
#
# "-j3" tells scons to run 3 build threads.  A good rule
# of thumb for choosing the -j option is to count the
# the number of CPU cores and add 1.  Remember you must
# be running and SMP kernel to take advantage of multi-
# core CPUs.
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

SetOption('warn', ['no-duplicate-environment'] + GetOption('warn'))

# we need to specify an absolute path so this SConstruct file
# can be called from any other SConstruct file
cwd = os.getcwd()

# non-java builds
buildList = """
                         app-defaults
                         acq
                         adm
                         Diffusion
                         fidlib
                         fiddle
                         fonts
                         Gmap
                         IMAGE_patent
                         Pbox
                         pipe
                         PFG
                         misc 
                         protune
                         shims
                         shimmethods
                         studylib
                         sudo.lnx
                         xml
                         xmllayout
                         """.split();

#  Done in common
#                        BIR
#                        execpars
#                         Gxyz_i
#                        nuctables
#                        maclib
#                        manual
#                        menujlib
#                        modules 
#                        mollib
#                        msg
#                        personalib
#                        probes
#                        proshimmethods
#                        satellites
#                        shapelib
#                        tablib
#                        templates
#                        upar
#                        user_templates
#                        vnmrbg_iconlib
#                        nvpsg  done by psglib
#                        psg  done by psglib
#                        ncomm  done by psglib
#                        kpsg  done by kpsglib

buildList = """
                         vnmrbg
                         3D
                         autotest
                         backproj
                         bin
                         biopack
                         biosolidspack
                         common
                         dicom_store
                         DOSY
                         ddr
                         Gilson
                         gxyzshim
                         IMAGE
                         languages
                         layouts
                         license
                         LCNMR
                         p11
                         passwd
                         psglib
                         roboproc
                         scripts
                         shuffler
                         solidspack
                         stars
                         tcl
                         veripulse
                         """.split();

acqBuildList = """
                         768AS
                         craft
                         Cryo2
                         CSI2
                         dicom
                         fdm
                         FDM2
                         kermit
                         nvacq
                         nvacqkernel
                         nvdsp
                         servicetools
                         """.split();

acqBuildList = """
                         ampfit
                         atproc
                         bootpd.rh51
                         cgl
                         ddl
                         dicom_fdf
                         expproc
                         ib
                         infoproc
                         inova
                         kpsglib
                         lcpeaks
                         mercury
                         nautoproc
                         nvexpproc
                         nvinfoproc 
                         nvrecvproc
                         nvsendproc
                         procproc
                         masproc
                         recvproc
                         sendproc
                         stat
                         web
                         """.split();

gslBuildList = """
                         aslmirtime
                         bin_image
                         xrecon
                         """.split();

# N.B. probeid was being built by apt, and vjclient by probeid.
#      SCons shortcomings have forced the use of hardlinks.
#
javaBuildList = """
                             admin
                             dialog
                             jaccount
                             jplot
                             managedb
                             vjmol
                             vnmrj
                             """.split();

javaAcqBuildList = """
                             apt
                             cryo
			     cryomon
                             probeid
                             """.split();

thirdPartyList = """
                              JavaPackages
                              """.split();

# print("Update versions file's keyword __GITDESCRIBE__ if present"
#  copy Version to the versions file so if it has the __GITDESCRBE__ key it
#  will be replaced. For releases, the File <git-repo>/scripts/ReleaseVersion 
#  is copied to <git-repo>/Version and it does not have the __GITDESCRIBE__
#  in it. This next step then copies Version to versions.
# command = 'cp ../Version ../versions; cd ../scripts; ./updateVersions.sh'
# idproc = subprocess.Popen( command, shell=True)
# status = os.waitpid(idproc.pid, 0)


javaLink = os.path.join(ovjtools, 'java')
# this must come last, since it creates sha1sum for all files

for i in buildList:
   SConscript(os.path.join('src',i, 'SConstruct'))

# Check for link to java home on Linux only (darwin uses the System java)
if os.path.exists(javaLink) or 'linux' not in platform:
   for i in javaBuildList:
      SConscript(os.path.join('src',i, 'SConstruct'))
else:
   print("java link in "+ovjtools+" not found. Skipping java compiles")

if ( os.path.exists(os.path.join('/usr','include','gsl')) ):
   for i in gslBuildList:
      SConscript(os.path.join('src',i, 'SConstruct'))
else:
   print("gsl includes not found. Skipping compiles requiring gsl")

vnmrPath    = os.path.join(cwd, os.pardir,'vnmr')

if ( 'darwin' not in platform):
   for i in acqBuildList:
      SConscript(os.path.join('src',i, 'SConstruct'))

   if os.path.exists(javaLink):
      for i in javaAcqBuildList:
         SConscript(os.path.join('src',i, 'SConstruct'))

   wkLink = os.path.join(ovjtools, 'wkhtmltopdf')
   if os.path.exists(wkLink):
      binPath = os.path.join(vnmrPath, 'bin')
      if not os.path.exists(binPath):
         os.makedirs(binPath)
      cmd = 'cp '+wkLink+'/wkhtmltopdf '+binPath+';chmod 755 '+binPath+'/wkhtmltopdf'
#     print("cmd: ",cmd)
      os.system(cmd)
#     cmd = 'cp '+wkLink+'/wkhtmltopdf-i386 '+binPath+';chmod 755 '+binPath+'/wkhtmltopdf-i386'
#     print("cmd: ",cmd)
#     os.system(cmd)

# end of if platform group

# for i in thirdPartyList:
#    SConscript(os.path.join('3rdParty', i, 'SConstruct'))

# this one is separate
# if ( 'darwin' not in platform):
#    SConscript(os.path.join(cwd, os.pardir, '3rdParty', 'SConstruct'))


# define  absolute  path  for  acqqueue  and  tmp  directory
# vnmracqueuPath    = os.path.join(cwd, os.pardir, 'vnmr','acqqueue')
# vnmrtmpPath       = os.path.join(cwd, os.pardir, 'vnmr','tmp')
# make sure the path(s) exist
# if not os.path.exists(vnmracqueuPath):
#    os.makedirs(vnmracqueuPath)
# if not os.path.exists(vnmrtmpPath):
#    os.makedirs(vnmrtmpPath)
# os.chmod(vnmracqueuPath,0o777)
# os.chmod(vnmrtmpPath,0o777)

vnmrSha1Path = os.path.join(vnmrPath,'adm','sha1')
if not os.path.exists(vnmrSha1Path):
   os.makedirs(vnmrSha1Path)

def runSconsPostAction(dir):
   dirList = os.listdir(dir)
   for i in dirList:
      sconsFile = os.path.join(dir,i,'sconsPostAction')
      if os.path.exists(sconsFile):
         cmd='cd '+os.path.join(dir,i)+';chmod +x sconsPostAction; ./sconsPostAction; rm sconsPostAction'
         print("cmd: ",cmd)
         os.system(cmd)

# this must come last, since it creates sha1sum for all files
def afterScons():
   runSconsPostAction(vnmrPath)

   print("Build ID file")
   command = 'cd scripts; ./genBuildId.pl'
   idproc = subprocess.Popen( command, shell=True)
   status = os.waitpid(idproc.pid, 0)

   print("Build Sha1 SnapShot of files")
   command = 'cd scripts; ./createSha1ChkList.sh'
   idproc = subprocess.Popen( command, shell=True)
   status = os.waitpid(idproc.pid, 0)

import atexit
atexit.register(afterScons)
