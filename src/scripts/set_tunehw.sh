#!/bin/sh
probeid=`/vnmr/bin/probeid -id`
cache=/vnmr/probeid/cache/$probeid
tunedir=$cache/Varian/Tune.d
tunelink=$cache/Varian/Tune
tunelinked=`readlink $tunelink`
tunesel=`basename $tunelinked`
options=`ls -1 $tunedir | awk "{printf(\"%s %s \", (\"$tunesel\" == \\$1) ? \"TRUE\" : \"FALSE\", \\$1)}"`

if [[ ! -e $tunedir ]]; then exit 0; fi

selection=`/usr/bin/zenity --title "Choose a Polarity Tuning Configuration" --list --radiolist --column "Select" --column="Tuning Hardware" $options`

if [[ ! $? -eq 1 && $selection != '' ]]; then
  (cd $tunedir/.. && rm -f Tune && /bin/ln -s Tune.d/$selection Tune)
fi

tunesel=''
tunelinked=`readlink $tunelink` && tunesel=`basename $tunelinked`
echo $tunesel
