#!/bin/csh
setawk

set Npts = 41

set Npts_real = `expr $Npts \* 8 `
set Npts_compl = `expr $Npts \* 16 `

var2pipe -in ./fid -noaswap  \
  -xN               754  -yN               $Npts_compl  \
  -xT               377  -yT               $Npts_real  \
  -xMODE        Complex  -yMODE  Echo-AntiEcho  \
  -xSW         7530.120  -ySW         1600.000  \
  -xOBS         599.667  -yOBS          60.771  \
  -xCAR           4.677  -yCAR         117.789  \
  -xLAB              HN  -yLAB             N15  \
  -ndim               2  -aq2D          States  \
  -out ./test.fid -verb -ov

sethdr test.fid -yT $Npts
