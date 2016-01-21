#! /bin/csh
# report back to VJ once the cln3d calculations are finished
# $1 is destination file path name
# $2 is VJ port ID

ps -A | grep cln3d > $1/stop.txt

while ((-f $1/fid)== 0)
  sleep 2
end

rm $1/stop.txt

Vnmrbg -mback -n0 "write('net','$HOST',$2,'write(\'alpha\',\'cln3d done.\')\n')"
Vnmrbg -mback -n0 "write('net','$HOST',$2,'write(\'line3\',\'cln3d done.\')\n')"

