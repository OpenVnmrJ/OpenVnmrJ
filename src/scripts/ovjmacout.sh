#!/bin/sh
#
#
# Copyright (C) 2015  University of Oregon
# 
# You may distribute under the terms of either the GNU General Public
# License or the Apache License, as specified in the LICENSE file.
# 
# For more information, see the LICENSE file.
# 

# Default Declarations
#

if [ "x${workspacedir}" = "x" ]
then
   workspacedir=$HOME
fi
if [ "x${dvdBuildName1}" = "x" ]
then
   dvdBuildName1=dvdimageOVJ
fi

gitdir="${workspacedir}/OpenVnmrJ"
vnmrdir="${gitdir}/../vnmr"
standardsdir="${gitdir}/../options/standard"
optddrdir="${gitdir}/../options/console/ddr"
ddrconsoledir="${gitdir}/../console/ddr"

if [ x$ovjAppName = "x" ]
then
   ovjAppName=OpenVnmrJ.app
fi

# TODO replace this with spiffy OS X app that does away with installer...
packagedir="${workspacedir}/${dvdBuildName1}/Package_contents"
resdir="${workspacedir}/${dvdBuildName1}/install_resources"
mkdir -p "${packagedir}"
mkdir "${resdir}"

cd "${gitdir}/src/macos"
cp -r VnmrJ.app "${packagedir}/$ovjAppName"
rm -f VJ
cc -Os -mmacosx-version-min=10.8 -arch i386 -arch x86_64 VJ.c -o VJ
mkdir -p "${packagedir}/$ovjAppName/Contents/MacOS"
cp VJ "${packagedir}/$ovjAppName/Contents/MacOS/."
rm -f "${vnmrdir}/bin/convert"
#cp convert $vnmrdir/bin/.
tar jxf ImageMagick.tar.bz2 -C $vnmrdir
# rm -rf "${vnmrdir}/jre"
# cp $JAVA_HOME/jre $vnmrdir/
rm -rf $vnmrdir/pgsql
cp -a "${OVJ_TOOLS}/pgsql.osx" "${vnmrdir}"/pgsql

vjdir="${packagedir}/${ovjAppName}/Contents/Resources/OpenVnmrJ"

mkdir -p "${vjdir}"
mkdir "${vjdir}/tmp" "$vjdir/acqqueue"
chmod 777 "${vjdir}/tmp" "$vjdir/acqqueue"

# backup for later copying useful dirs in existing /vnmr
preinstall=$resdir/preinstall
printf "#!/bin/sh\n" > $preinstall
printf "orig=\`readlink /vnmr\`\n" >> $preinstall
printf "if [ -d \$orig/fidlib ]\n" >> $preinstall
printf "then\n" >> $preinstall
printf "  (cd \$orig; zip -ryq fidlib.zip fidlib; mv fidlib.zip /tmp )\n" >> $preinstall
printf "fi\n" >> $preinstall
printf "if [ -d \$orig/help ]\n" >> $preinstall
printf "then\n" >> $preinstall
printf "  (cd \$orig; zip -ryq help.zip help; mv help.zip /tmp )\n" >> $preinstall
printf "fi\n" >> $preinstall
printf "if [ -d \$orig/nmrPipe ]\n" >> $preinstall
printf "then\n" >> $preinstall
printf "  (cd \$orig; zip -ryq nmrpipe.zip nmrPipe; mv nmrpipe.zip /tmp )\n" >> $preinstall
printf "fi\n" >> $preinstall
printf "rm -rf /Applications/$ovjAppName\n" >> $preinstall
chmod +x $preinstall

# setup the OpenVnmrJ environment and re-install any directories backup up above
postinstall=$resdir/postinstall
printf "#!/bin/sh\n" > $postinstall
printf "rm -rf /vnmr\n" >> $postinstall
printf "ln -s /Applications/$ovjAppName/Contents/Resources/OpenVnmrJ /vnmr\n" >> $postinstall
printf "username=\$(/usr/bin/stat -f%%Su /dev/console)\n" >> $postinstall
printf "echo $%s > /vnmr/adm/users/userlist\n" username >> $postinstall
printf "echo \"vnmr:VNMR group:$%s\" > /vnmr/adm/users/group\n" username >> $postinstall
printf "cd /vnmr/adm/users/profiles/system; sed s/USER/$%s/g < sys_tmplt  > $%s; rm -f sys_tmplt\n" username username >> $postinstall
printf "cd /vnmr/adm/users/profiles/user;   sed s/USER/$%s/g < user_tmplt > $%s; rm -f user_tmplt\n" username username >> $postinstall
printf "chown -h $%s /vnmr\n" username >> $postinstall
printf "chown -R -L $%s /vnmr\n" username >> $postinstall
printf "/vnmr/bin/makeuser $%s y\n" username >> $postinstall
printf "/vnmr/bin/dbsetup root /vnmr \n" >> $postinstall

printf "if [ -f /tmp/fidlib.zip ]\n" >> $postinstall
printf "then\n" >> $postinstall
printf "  mv /tmp/fidlib.zip /vnmr\n" >> $postinstall
printf "  (cd /vnmr; unzip -q fidlib.zip; rm -f fidlib.zip)\n" >> $postinstall
printf "fi\n" >> $postinstall
printf "if [ -f /tmp/help.zip ]\n" >> $postinstall
printf "then\n" >> $postinstall
printf "  mv /tmp/help.zip /vnmr\n" >> $postinstall
printf "  (cd /vnmr; unzip -q help.zip; rm -f help.zip)\n" >> $postinstall
printf "fi\n" >> $postinstall
printf "if [ -f /tmp/nmrpipe.zip ]\n" >> $postinstall
printf "then\n" >> $postinstall
printf "  mv /tmp/nmrpipe.zip /vnmr\n" >> $postinstall
printf "  (cd /vnmr; unzip -q nmrpipe.zip; rm -f nmrpipe.zip)\n" >> $postinstall
printf "fi\n" >> $postinstall

chmod +x $postinstall

cd "$vnmrdir"; tar cf - --exclude .gitignore --exclude "._*" . | (cd "$vjdir"; tar xpf -)
cd "$ddrconsoledir"; tar cf - --exclude .gitignore --exclude "._*" . | (cd "$vjdir"; tar xpf -)

optionslist=`ls $standardsdir`
for file in $optionslist
do
   if [ $file != "P11" ]
   then
      cd "${standardsdir}/${file}"
      tar cf - --exclude .gitignore --exclude "._*" . | (cd "${vjdir}"; tar xpf -)
   fi
done
optionslist=`ls $optddrdir`
for file in $optionslist
do
   if [ $file != "P11" ]
   then
      cd $optddrdir/$file
      tar cf - --exclude .gitignore --exclude "._*" . | (cd $vjdir; tar xpf -)
   fi
done

cd $gitdir/src/macos
rm -f $vjdir/bin/vnmrj
cp vnmrj.sh $vjdir/bin/vnmrj
chmod 755 $vjdir/bin/vnmrj

echo "vnmrs" >> $vjdir/vnmrrev 
cd $vjdir/adm/users
cat userDefaults | sed '/^home/c\
home    yes     no      /Users/$accname\
' > userDefaults.bak
rm -f userDefaults
mv userDefaults.bak userDefaults
mkdir profiles/system profiles/user
cp "${gitdir}/src/macos/sys_tmplt" profiles/system/.
cp "${gitdir}/src/macos/user_tmplt" profiles/user/.

#Following need a certificate issued by Apple to developer
echo "Code signing requires a certificate issued by Apple. Check for any errors and fix for OS X El Capitan or macOS Sierra"
codesign -s "3rd Party Mac Developer Application:" --entitlements "${gitdir}/src/macos/entitlement.plist" "${packagedir}/${ovjAppName}"
