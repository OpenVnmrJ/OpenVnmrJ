import os

#get current working directory
cwd = os.getcwd()


# the file list fro this bunch
fileList =     ['create_pgsql_user']


#  it tp vnmr/pgsql.lnx/bin
vnmrBinPath = os.path.join(cwd,os.pardir,os.pardir,os.pardir,'vnmr','pgsql','bin')
if not os.path.exists(vnmrBinPath) :
      os.makedirs(vnmrBinPath)
   
for i in fileList:
      dest = os.path.join(vnmrBinPath,i)
      Execute(Copy(dest,i+'.sh'))
      Execute(Chmod(dest,0755))

