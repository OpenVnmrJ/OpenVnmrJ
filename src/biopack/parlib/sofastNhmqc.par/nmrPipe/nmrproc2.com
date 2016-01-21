#!/bin/csh

# Processing and addition of both sub spectra of the SE IPAP SOFAST experiment
# Dont' forget to adjust the spectral width SWH and the number of points npts of the direct dimension
# The coupling constant presumed for the N-H coupling is 95Hz and can be changed in the phi1p exoression 

set SWH  = 10040
set npts = 502
set phi1p = `expr 360 \* 95 / 2 \* $npts / $SWH`
set phi1n = `expr -1 \* $phi1p`
echo $phi1p

set ap = (\
                0 1\
                0 1\
                1 0\
                1 0\
                )

set ip = (\
                1 0\
                -1 0\
                0 -1\
                0 1\
                )



nmrPipe -in data/test.fid \
| nmrPipe  -fn QMIX  -ic 4 -oc 2 -cList $ap\
| nmrPipe  -fn PS -p0 90 -p1 $phi1p              \
   -verb -ov -out APs.fid

nmrPipe -in data/test.fid \
| nmrPipe  -fn QMIX  -ic 4 -oc 2 -cList $ip\
| nmrPipe  -fn PS -p1 $phi1p              \
   -verb -ov -out IPs.fid


addNMR -in1 IPs.fid -in2 APs.fid  -c2 1 -add  -out sum.fid


nmrPipe -in data/test.fid \
| nmrPipe  -fn QMIX  -ic 4 -oc 2 -cList $ap\
| nmrPipe  -fn PS -p0 90 -p1 $phi1n              \
   -verb -ov -out APs.fid

nmrPipe -in data/test.fid \
| nmrPipe  -fn QMIX  -ic 4 -oc 2 -cList $ip\
| nmrPipe  -fn PS -p1 $phi1n              \
   -verb -ov -out IPs.fid

addNMR -in1 IPs.fid -in2 APs.fid  -c2 1 -sub  -out diff.fid

addNMR -in1 sum.fid -in2 diff.fid  -c2 1 -add  -out final.fid


nmrPipe -in final.fid \
| nmrPipe  -fn POLY -time                             \
| nmrPipe  -fn SP -off 0.5 -end 0.98 -pow 2 -c 0.5    \
| nmrPipe  -fn ZF -pad 1024                               \
| nmrPipe  -fn FT                                     \
| nmrPipe  -fn PS -p0 0 -p1 0 -di                 \
| nmrPipe  -fn EXT -left -sw -verb                    \
| nmrPipe  -fn TP                                     \
| nmrPipe  -fn SP -off 0.5 -end 0.98 -pow 1 -c 0.5    \
| nmrPipe  -fn ZF -pad 512                              \
| nmrPipe  -fn FT                                     \
| nmrPipe  -fn PS -p0 0 -p1 0 -di                \
| nmrPipe  -fn POLY -auto                        \
| nmrPipe  -fn TP                                     \
| nmrPipe  -fn POLY -auto                        \
   -verb -ov -out final.ft2

