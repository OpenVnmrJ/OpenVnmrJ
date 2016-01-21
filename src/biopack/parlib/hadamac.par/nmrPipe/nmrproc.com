#!/bin/csh

#
# Basic 2D Phase-Sensitive Processing:
#   Cosine-Bells are used in both dimensions.
#   Use of "ZF -auto" doubles size, then rounds to power of 2.
#   Use of "FT -auto" chooses correct Transform mode.
#   Imaginaries are deleted with "-di" in each dimension.
#   Phase corrections should be inserted by hand.

set M1 = (\
                1 0\
		0 1\
                0 0\
		0 0\
                0 0\
		0 0\
                0 0\
		0 0\
                0 0\
		0 0\
                0 0\
		0 0\
                0 0\
		0 0\
                0 0\
		0 0\
                )
set M2 = (\
                0 0\
		0 0\
                1 0\
		0 1\
                0 0\
		0 0\
                0 0\
		0 0\
                0 0\
		0 0\
                0 0\
		0 0\
                0 0\
		0 0\
                0 0\
		0 0\
                )

set M3 = (\
                0 0\
		0 0\
                0 0\
		0 0\
                1 0\
		0 1\
                0 0\
		0 0\
                0 0\
		0 0\
                0 0\
		0 0\
                0 0\
		0 0\
                0 0\
		0 0\
                )

set M4 = (\
                0 0\
		0 0\
                0 0\
		0 0\
                0 0\
		0 0\
                1 0\
		0 1\
                0 0\
		0 0\
                0 0\
		0 0\
                0 0\
		0 0\
                0 0\
		0 0\
                )


set M5 = (\
                0 0\
		0 0\
                0 0\
                0 0\
		0 0\
		0 0\
                0 0\
		0 0\
                1 0\
		0 1\
                0 0\
		0 0\
                0 0\
		0 0\
                0 0\
		0 0\
                )
set M6 = (\
                0 0\
		0 0\
                0 0\
                0 0\
		0 0\
                0 0\
		0 0\
		0 0\
                0 0\
		0 0\
                1 0\
		0 1\
                0 0\
		0 0\
                0 0\
		0 0\
                )

set M7 = (\
                0 0\
		0 0\
                0 0\
                0 0\
		0 0\
                0 0\
		0 0\
                0 0\
		0 0\
		0 0\
                0 0\
		0 0\
                1 0\
		0 1\
                0 0\
		0 0\
                )


set M8 = (\
                0 0\
		0 0\
                0 0\
                0 0\
		0 0\
                0 0\
		0 0\
                0 0\
		0 0\
		0 0\
                0 0\
		0 0\
                0 0\
		0 0\
                1 0\
		0 1\
                )
set GLY = ()
set ASN = ()
set CYS = ()
set ALA = ()
set SER = ()
set THR = ()
set REST = ()


set C =( 1 1 1 1 1 1 1 1 )
#set C =( 1 0 0 0 0 0 0 0 )


foreach i (1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32)
	set val =  `expr $M1[$i] - $M2[$i] - $M3[$i] + $M4[$i] - $M5[$i] + $M6[$i] + $M7[$i] - $M8[$i]`	
	set GLY = ($GLY $val)

	set val =  `expr $M1[$i] - $M2[$i] - $M3[$i] + $M4[$i] + $M5[$i] - $M6[$i] - $M7[$i] + $M8[$i]`
	set ASN = ($ASN $val)

	set val =  `expr $M1[$i] + $M2[$i] - $M3[$i] - $M4[$i] + $M5[$i] + $M6[$i] - $M7[$i] - $M8[$i]`
	set CYS = ($CYS $val)

	set val =  `expr $M1[$i] + $M2[$i] - $M3[$i] - $M4[$i] - $M5[$i] - $M6[$i] + $M7[$i] + $M8[$i] `
	set ALA = ($ALA $val)

	set val =  `expr $M1[$i] + $M2[$i] + $M3[$i] + $M4[$i] + $M5[$i] + $M6[$i] + $M7[$i] + $M8[$i]`
	set SER = ($SER $val)

	set val =  `expr -1 \* $M1[$i] - $M2[$i] - $M3[$i] - $M4[$i] + $M5[$i] + $M6[$i] + $M7[$i] + $M8[$i]`
	set THR = ($THR $val)


	set val =  `expr $M1[$i] - $M2[$i] + $M3[$i] - $M4[$i] - $M5[$i] + $M6[$i] - $M7[$i] + $M8[$i]`
	set REST = ($REST $val)

end

foreach list ( GLY ASN  CYS ALA  SER THR REST)
if ($list == 'GLY' )     set mat = ($GLY)    end
if ($list == 'ASN' )     set mat = ($ASN)    end
if ($list == 'CYS' )  set mat = ($CYS) end
if ($list == 'ALA' )     set mat = ($ALA)    end
if ($list == 'SER' )     set mat = ($SER)    end
if ($list == 'THR' )     set mat = ($THR)    end
if ($list == 'REST' )    set mat = ($REST)   end

#end
#foreach list (1 2 3 4 5 6 7 8)

if ($list == 1 )     set mat = ($M1) end
if ($list == 2 )     set mat = ($M2) end
if ($list == 3 )     set mat = ($M3) end
if ($list == 4 )     set mat = ($M4) end
if ($list == 5 )     set mat = ($M5) end
if ($list == 6 )     set mat = ($M6) end
if ($list == 7 )     set mat = ($M7) end
if ($list == 8 )     set mat = ($M8) end

nmrPipe -in test.fid \
| nmrPipe  -fn QMIX -ic 16 -oc 2 -cList $mat \
| nmrPipe  -fn SOL \
| nmrPipe  -fn SP -off 0.5 -end 1.00 -pow 2 -c 1.0    \
| nmrPipe  -fn ZF -auto                               \
| nmrPipe  -fn FT -auto                               \
| nmrPipe  -fn PS -p0 93.20 -p1 15.00 -di -verb         \
| nmrPipe  -fn EXT -left  -sw\
#| nmrPipe  -fn EXT -left -sw\
| nmrPipe  -fn TP                                     \
| nmrPipe  -fn LP -ps0-0 -pred 50\
| nmrPipe  -fn SP -off 0.7 -end 0.98 -pow 2 -c 0.5    \
| nmrPipe  -fn ZF -auto                               \
| nmrPipe  -fn FT -auto                               \
| nmrPipe  -fn PS -p0 0.00 -p1 0.00 -di -verb         \
| nmrPipe  -fn TP                                     \
| nmrPipe  -fn POLY -auto                                     \
| nmrPipe  -fn TP                                     \
   -ov -out $list.ft2

nmrPipe -in  $list.ft2   \
|  nmrPipe -fn TP       \
| pipe2xyz -out hadamac_$list.nv -ov -nv

end
