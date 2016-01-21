import os
import shutil

# method that creates multiple links as a dependency of a build target
# typical case is fileList is filea, name is _ddr, and ext is .c
# linking filea_ddr.c to filea.c
def appendSymLinks(env, buildTarget, destPath, sourcePath, fileList, name, ext):
   for i in fileList:
      # create a link
      linkTargetFile = os.path.join(destPath, i+name+ext)
      linkSourceFile = os.path.join(sourcePath, i+ext)
      string = 'ln -fs ' + linkSourceFile + ' ' + linkTargetFile
      env.Command(target = linkTargetFile,
                  source = linkSourceFile,
                  action = string)
      # make sure link is made before target is built
      env.Depends(target     = linkTargetFile,
                  dependency = linkSourceFile)

# method that creates multiple links as a dependency of a build target
def makeLinks(env, moh, buildTarget, destPath, sourcePath, fileList):
   for i in fileList:
      # create a link
      linkTargetFile = os.path.join(destPath, i)
      linkSourceFile = os.path.join(sourcePath, i)
      hardSoft = ''
      if moh == 'symbolic':
         hardSoft = ' -s '
      string = 'ln -f ' + hardSoft + linkSourceFile + ' ' + linkTargetFile
      env.Command(target = linkTargetFile,
                  source = linkSourceFile,
                  action = string)

      # make sure link is made before target is built
      env.Depends(target     = linkTargetFile,
                  dependency = linkSourceFile)
      #env.Depends(target     = buildTarget,
      #            dependency = linkTargetFile)

# create symbolic links
def makeSymLinks(env, buildTarget, destPath, sourcePath, fileList):
   moh = 'symbolic'
   makeLinks(env, moh, buildTarget, destPath, sourcePath, fileList)

# create hard links
def makeHardLinks(env, buildTarget, destPath, sourcePath, fileList):
   moh = ''
   makeLinks(env, moh, buildTarget, destPath, sourcePath, fileList)

# Method to iterate over a list of SOURCE files creating symlinks,
# compiling the source, and explicitly mapping the dependency betweeen
# the pre-linked source content and the compiled object in the local
# directory.  Returns aggregates list of static object build objects.
def symLinkStaticObj(env, destPath, sourcePath, fileList):
   returnTempObjs = []
   for i in fileList:
      # create absolute paths to files
      linkTargetFile = os.path.join(destPath, i)
      linkSourceFile = os.path.join(sourcePath, i)

      # create symlink
      env.Command(target = linkTargetFile,
                  source = linkSourceFile,
                  action = 'ln -sf ' + linkSourceFile + ' ' + linkTargetFile)

      # build static object
      tempObj = env.StaticObject(source = linkTargetFile)

      # make sure link is made before target is built
      env.Depends(target     = tempObj,
                  dependency = linkTargetFile)

      # aggregate this build object to list
      returnTempObjs.append(tempObj)

   # return aggregated list of objects
   return returnTempObjs

# Symlinks in remote headers and maps dependency between each header file
# and a c source list.  SCons will do the correct dependency mapping for
# build purposes, we just have to make sure all the headers are symlinked
# in before any of the C source is compiled.
def symLinkHeader(env, cSourceFileList, destPath, headerSourcePath, headerFileList):
   for i in headerFileList:
      # create absolute paths to files
      linkTargetFile = os.path.join(destPath, i)
      linkSourceFile = os.path.join(headerSourcePath, i)

      # create symlink
      env.Command(target = linkTargetFile,
                  source = linkSourceFile,
                  action = 'ln -sf ' + linkSourceFile + ' ' + linkTargetFile)

      # make explicit dependency between C file list and header list (after symlinking)
      for k in cSourceFileList:
         env.Depends(target     = linkTargetFile,
                     dependency = k)

# method that creates links at interpretation time with no association with a build target
# this shouldn't be used if it can be helped because this will execute during a "clean"
#def linkNow(env, destPath, sourcePath, fileList):
#   for i in fileList:
#      linkTargetFile = os.path.join(destPath, i)
#      linkSourceFile = os.path.join(sourcePath, i)
#      env.Execute(action = 'ln -sf ' + linkSourceFile + ' ' + linkTargetFile)

def linkNow(env, moh, destPath, sourcePath, fileList):
   # make sure the destination path exists
   if not os.path.exists(destPath):
      os.makedirs(destPath)

   for i in fileList:
      linkTargetFile = os.path.join(destPath, i)
      linkSourceFile = os.path.join(sourcePath, i)
      hardSoft = ''
      if moh == 'symbolic':
         hardSoft = ' -s '
      string = 'ln -f ' + hardSoft + linkSourceFile + ' ' + linkTargetFile
      env.Execute(action = string)

# create symbolic links immediately
def symLinkNow(env, destPath, sourcePath, fileList):
   moh = 'symbolic'
   linkNow(env, moh, destPath, sourcePath, fileList)

# create hard links immediately
def hardLinkNow(env, destPath, sourcePath, fileList):
   moh = ''
   linkNow(env, moh, destPath, sourcePath, fileList)


# method to copy files after a target is built
def postBuildCopy(env, buildTarget, destPath, sourcePath, fileList):
   for i in fileList:
      # copy file
      copyTargetFile = os.path.join(destPath, i)
      copySourceFile = os.path.join(sourcePath, i)
      env.Command(target = copyTargetFile,
                  source = copySourceFile,
                  action = 'cp -f ' + copySourceFile + ' ' + copyTargetFile)

      # make sure copy is done after target is built
      env.Depends(target     = buildTarget,
                  dependency = copyTargetFile)

#      # add copied file to clean
#      env.Clean(buildTarget, targetFile)

# method to copy files after a target is built
def postBuildCopyPerm(env, buildTarget, destPath, sourcePath, fileList, perm):
   for i in fileList:
      # copy file
      copyTargetFile = os.path.join(destPath, i)
      copySourceFile = os.path.join(sourcePath, i)
      env.Command(target = copyTargetFile,
                  source = copySourceFile,
                  action = 'cp -f ' + copySourceFile + ' ' + copyTargetFile + '; chmod ' + perm + ' ' + copyTargetFile)

      # make sure copy is done after target is built
      env.Depends(target     = buildTarget,
                  dependency = copyTargetFile)

    
def copytree(src, dest, symlinks=False):
       """copyTree which does not fail if the directory exists.
       
       Recursively copy a directory tree using copy2().
   
       If the optional symlinks flag is true, symbolic links in the
       source tree result in symbolic links in the destination tree; if
       it is false, the contents of the files pointed to by symbolic
       links are copied.
       
       Behavior is meant to be identical to GNU 'cp -R'.    
       """
       def copyItems(src, dest, symlinks=False):
           """Function that does all the work.
           
           It is necessary to handle the two 'cp' cases:
           - destination does exist
           - destination does not exist
           
           See 'cp -R' documentation for more details
           """
           for item in os.listdir(src):
              srcPath = os.path.join(src, item)
              if os.path.isdir(srcPath):
                  srcBasename = os.path.basename(srcPath)
                  destDirPath = os.path.join(dest, srcBasename)
                  if not os.path.exists(destDirPath):
                      os.makedirs(destDirPath)
                  copyItems(srcPath, destDirPath)
              elif os.path.islink(item) and symlinks:
                  linkto = os.readlink(item)
                  os.symlink(linkto, dest)
              else:
                  shutil.copy2(srcPath, dest)
   
       # case 'cp -R src/ dest/' where dest/ already exists
       if os.path.exists(dest):
          destPath = os.path.join(dest, os.path.basename(src))
          if not os.path.exists(destPath):
              os.makedirs(destPath)
       # case 'cp -R src/ dest/' where dest/ does not exist
       else:
          os.makedirs(dest)
          destPath = dest
       # actually copy the files
       copyItems(src, destPath)

