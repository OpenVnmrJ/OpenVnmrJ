#! /bin/csh
# Script used to run background automation. Called from Autoproc.

source $HOME/.vnmrenv
Vnmrbg -mauto -hAutoproc -n1 -i42 -u$vnmruser autoqstart
