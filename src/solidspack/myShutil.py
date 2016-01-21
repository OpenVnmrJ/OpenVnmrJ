import os.path
import shutil
import os, errno    
def copytree(src, dest, symlinks=False):
       """My own copyTree which does not fail if the directory exists.
       
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


