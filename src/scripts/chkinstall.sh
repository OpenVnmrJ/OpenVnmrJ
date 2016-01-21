#!/bin/sh
# check the current files against the sha1 signitures from the original install
#
sha1sum -c sha1chklist.txt | grep FAIL
