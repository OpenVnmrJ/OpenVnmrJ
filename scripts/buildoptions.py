# Frits Vosman
#
# do not change this file, instead copy it up one directory level to 
# the git-repo directory, it is .gitignored there, so the defaults remain
#
# if you want to keep the default, leave it here as is (no need to copy)
#
# 'include' file for SConstruct files
#  Creates a boEnv (build-options Environment)
#  if needed, inlcude with:
#      # get options settings
#      boFile=os.path.join(cwd,os.pardir,...,'buildoptions.py')
#      if not os.path.exists(boFile):
#         boFile=os.path.join(cwd,os.pardir,...,'scripts','buildoptions.py')
#      execfile(boFile)
#  adjust number of os.pardir for ....  as needed.
#  Then use to test:
#      if boEnv['dasho'] == 'y':
#  etc
#  Add more values as needed 
#

vars = Variables()
boEnv = Environment(variables = vars,
                apt_0='n',
                AS768='n',
                BACKPROJ='y',
                BIR='n',
                CHEMPACK='n',
                cryomon_O='n',
                CSI='n',
                dasho='n',
                DOSY='n',
                DIFFUS='n',
                FDM='n',
                FIDDLE='n',
                GMAP='n',
                Gxyz='n',
                IMAGE='n',
                INOVA='n',
                jaccount_O='n',
                JCP='n',
                JMOL='n',
                LC='n',
                managedb_O='n',
                MR400FH='n',
                NDDS_VERSION='4.2e',
                NDDS_LIB_VERSION='i86Linux2.6gcc3.4.3',
                P11='n',
                PATENT='n',
                PFG='n',
                PROTUNE='n',
                protune_0='n',
                probeid_0='n',
                SENSE='n',
                SOLIDS='Y',
                stars='n',
                VAST='n',
                vnmrj_O='n',
		)


