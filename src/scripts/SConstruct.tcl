from __future__ import print_function
import os


print('SConstruct.768AS')
#get current working directory
cwd = os.getcwd()

# the file list fro this bunch
fileList =   [ 'at',
	       'atrecord',
               'atregbuilt' ]

# for sure copy it to options/768AS/bin
vnmrTclBinPath = os.path.join(cwd,os.pardir,os.pardir,os.pardir,
		'vnmr','tcl','bin')
if not os.path.exists(vnmrTclBinPath) :
    os.makedirs(vnmrTclBinPath)

for i in fileList:
   dest = os.path.join(vnmrTclBinPath,i)
   Execute(Copy(dest,i+'.sh'))
   Execute(Chmod(dest,0o755))

