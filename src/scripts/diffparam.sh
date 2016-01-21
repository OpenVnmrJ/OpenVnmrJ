#!/bin/sh
#
# diffparam - report differences between two parameter files
#
# diffparam file1 file2 <parameter_goup>
#
# started 95/03/25, r.kyburz

# +--------------------+
# | evaluate arguments |
# +--------------------+
if [ $# -lt 2 -o $# -gt 3 ]; then
  echo "Usage:  diffparam file1 file2 <parameter_group>"
  exit 1
fi

if [ -d $1 ]; then
  file1=$1/procpar
else
  file1=$1
fi
if [ ! -f $file1 ]; then
  echo "diffparam:  file $file1 not found"
  exit 1
fi
if [ -d $2 ]; then 
  file2=$2/procpar 
else
  file2=$2
fi
if [ ! -f $file2 ]; then 
  echo "diffparam:  file $file2 not found"
  exit 1 
fi

if [ $# -eq 3 ]; then
  group=$3
else
  group=acquisition
fi

listparam $file1 $group | sort > /tmp/listpar$$.par1
listparam $file2 $group | sort > /tmp/listpar$$.par2
diff /tmp/listpar$$.par1 /tmp/listpar$$.par2 | grep '^[><]' | sort +1 -2 | \
	sed 's/^>/->/' | sed 's/^</<-/'
rm -f /tmp/listpar$$.par[12]
