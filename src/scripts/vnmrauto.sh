#!/bin/bash
# Script used to run background automation. Called from Autoproc.

source $HOME/.vnmrenvsh
Vnmrbg -mauto -hAutoproc -n1 -i42 -u$vnmruser autoqstart
