from __future__ import print_function
import os
import sys
#get current working directory
cwd = os.getcwd()
ovjtools=os.getenv('OVJ_TOOLS')
if not ovjtools:
# If not defined, try the default location
    print("OVJ_TOOLS env not found. Trying default location.")
    ovjtools = os.path.join(cwd, os.pardir, os.pardir, os.pardir, 'ovjTools')

if not os.path.exists(ovjtools):
    print("OVJ_TOOLS env not found.")
    print("For bash and variants, use export OVJ_TOOLS=<path>")
    print("For csh and variants,  use setenv OVJ_TOOLS <path>")
    sys.exit(1)


installPath = os.path.join(cwd,os.pardir,os.pardir,os.pardir,'vnmr','pgsql')
Execute(Mkdir(installPath))

# MAC -> darwin, Linux -> linux2
platform = sys.platform
if ('darwin' in platform):
      Execute('cd '+ovjtools+'/pgsql.osx;tar zcf - * | (cd '+installPath+'; tar zxfp -)')
else:
      Execute('cd '+ovjtools+'/pgsql;tar zcf - * | (cd '+installPath+'; tar zxfp -)')

configPath = os.path.join(installPath,'config')
Execute(Mkdir(configPath))
Execute(Chmod(configPath,0777))
