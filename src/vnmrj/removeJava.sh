#! /bin/sh
# it seems the SConstruct for java places the source code (*.java) files into the classes directory
# thus we need to remove these and rejar the vnmrj.jar
set -x
find classes -name "*.java" -exec rm -f {} \;
find classes -name "*.out" -exec rm -f {} \;
# the Sconstruct calls this script as a post action after the compilation of java
# then performs the jar operation
## rm vnmrj.jar
## jar cfm vnmrj.jar Manifest -C classes .
## ../../3rdParty/java/bin/jar cfm vnmrj.jar Manifest -C classes .
