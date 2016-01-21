#!/bin/csh

mkdir -p data

var2pipe -in ../fid -noaswap  \
  -xN              1004  -yN               200  \
  -xT               502  -yT                50  \
  -xMODE        Complex  -yMODE        Complex  \
  -xSW        10040.161  -ySW         1800.000  \
  -xOBS         799.975  -yOBS          81.070  \
  -xCAR           4.763  -yCAR         118.641  \
  -xLAB              1H  -yLAB             N15  \
  -ndim               2  -aq2D          States  \
  -out data/test.fid -verb -ov

