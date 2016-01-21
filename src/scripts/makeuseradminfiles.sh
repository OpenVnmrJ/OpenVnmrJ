#!/bin/ksh

# This script is ONLY for interix.  It is to create some of the admin
# files that are normally done when a user is created.

# Modified for adding non-admin users
# arg1 = User Name
# arg2 = User Home Dir

#default vnmrsystem to /vnmr if undefined
if [ x"$vnmrsystem" = "x" ]
then
   vnmrsystem="/vnmr"
fi

gethomedirInterix()
{
    home_dir=""
    if [ "x$osname" = "xInterix" ]
    then
#	home_dir=`/vnmr/bin/getuserinfo $1 | awk 'BEGIN { FS = ";" } {print $2}'`
        home_dir="$3"
        echo "gethomedirInterix:  home_dir : $home_dir"
#	home_dir=`$home_dir | sed -e 's@\\\\@\\\\\\\\@g'`
    fi
}

setdatadir()
{   
    oldIFS=$IFS
    if [ "x$osname" = "xInterix" ]
    then
	IFS=';'
    fi

    if [ "x$osname" = "xInterix" ]
    then
	datadir=`grep datadir $1 | sed -e 's@\\\\@\\\\\\\\@g'`
    else   
	datadir=`grep datadir $1`
    fi

    if [ -n "$datadir" ]
    then
	hasdatadir="y"
        
	for dir in $datadirline
	do
	    dirfound="false"
	    for dir2 in $datadir
	    do
	        if [ "x$dir2" = x"$dir" ]
	        then
		    dirfound="true"
	        fi
	    done
	    if [ "x$dirfound" = "xfalse" ]
	    then
		if [ "x$osname" = "xInterix" ]
		then
		    datadir="$datadir;$dir"
		else
		    datadir="$datadir $dir"
		fi
	    fi
	done   

	cat $1 | sed '/^datadir/c\
'"$datadir"'' > ${1}.bak
	mv ${1}.bak ${1}
    fi

    IFS=$oldIFS
}

makeadmfiles()
{
    seperator=""
echo "Top makeadmfiles()"
    #  userlist 
    file="$vnmrsystem"/adm/users/userlist
    if [ ! -f "$file" ]
    then
	echo "Creating $file"
	echo "$1" > "$file"
    else
echo "Found userlist"
        change_vnmr_adm="false"
        grep $1 "$file"
        if [ $? -ne 0 ]
        then
	    /bin/echo $1 >> "$file"
            /bin/cat "$file" | /usr/bin/tr '\n' ' ' > ${file}.new
            /bin/mv ${file}.new "$file"
            change_vnmr_adm="true"
        fi
    fi

    #make sure there is only one admin
    cd "$vnmrsystem"/adm/users/profiles/system/
# For non-admin users we don't want to do this
#    list=`grep  -l -s System *`
#    for item in $list
#    do
#	grep -v "System Administrator" $item > ${item}.new
#        echo "name     $item" >> ${item}.new
#        mv ${item}.new $item
#    done

    osname=`uname -s`
    vnmrdir="/vnmr"
    vnmrsysdir="$vnmrdir"
    # The users vnmrsys directory path
    user_dir="$4"
    slash="/"
    if [ "x$osname" = "xInterix" ]
    then
	# change the unix path to windows path, and replace \ with \\ to escape slash.
	# To escape one slash in sed it's \\\\.
	# Use unixpath2win for getting the actual path for softlinks
	vnmrdirInterix=`/bin/ntpath2posix "$vnmrdir"`
	user_dirInterix=`/bin/ntpath2posix "$user_dir"`
	vnmrsysdir=`/bin/unixpath2win "$vnmrdir" | sed -e 's@\\\\@\\\\\\\\@g'`
	vnmrdir=`/bin/posixpath2nt "$vnmrdir" | sed -e 's@\\\\@\\\\\\\\@g'`
	user_dir=`/bin/posixpath2nt "$user_dir" | sed -e 's@\\\\@\\\\\\\\@g'`
	slash="\\\\"
	if [ "x$2" != "xdb" ]
	then
	    vnmruser_dir="$user_dir"
	    vnmruser_dirInterix="$user_dirInterix"
	fi
        
	datadirline="${user_dir}${slash}data;${vnmrdir}${slash}fidlib;${vnmrdir}${slash}stdpar;${vnmrdir}${slash}tests;${vnmrdir}${slash}parlib;${vnmrdir}${slash}imaging${slash}tests;${vnmrdir}${slash}shims;${user_dir}${slash}parlib;${user_dir}${slash}shims"
    else
	datadirline="${user_dir}/data ${vnmrdir}/fidlib ${vnmrdir}/stdpar ${vnmrdir}/tests ${vnmrdir}/parlib ${vnmrdir}/imaging/tests ${vnmrdir}/shims ${user_dir}/parlib ${user_dir}/shims"
    fi

    # system
    file="$vnmrsystem"/adm/users/profiles/system/$1

    if [ ! -f "$file" ]
    then
	echo "Creating $file"
        echo "accname  $1" > "$file"
        echo "name     $1" >> "$file"
        # echo fails if path has some backslash characters, use print -R
        print -R "home     $3" >> "$file"
	if [ "x$osname" = "xInterix" ]
	then
	    if [ "x$2" != "xdb" ]
	    then
                print -R -n "owned    ${3}" >> "$file"
                echo ";${vnmrdir}" >> "$file"
	    else
		print -R -n "owned    ${3}" >> "$file"
	    fi
	else
	    echo "owned    ${3} ${vnmrsysdir}" >> "$file"
	fi
        echo "usrlvl   2" >> "$file"
        echo "access   all" >> "$file"
        if [ "x$APP_MODE" = "ximaging" ]
        then
	    echo "itype    Imaging" >> "$file"
	elif [ "x$APP_MODE" = "xwalkup" ]
        then
	    echo "itype    Spectroscopy" >> "$file"
        else
	    echo "itype    Spectroscopy" >> "$file"
        fi
#	echo "cmdArea    Yes" >> "$file"
# If file already exists, just leave it along
#    else
#        grep -v -w "name" "$file" > ${file}.new
#	if [ "x$2" != "xdb" ]
#	then
#	    echo "name     System Administrator" >> ${file}.new
#	fi
#        mv ${file}.new "$file"
	
#	setdatadir "$file"
        
    fi
    # user
    file="$vnmrsystem"/adm/users/profiles/user/$1
    if [ ! -f ""$file"" ]
    then
	echo "Creating $file"
	echo "userdir   ${user_dir}" > "$file"
	echo "sysdir    ${vnmrsysdir}" >> "$file"
        if [ "x$APP_MODE" = "ximaging" ]
        then
	    echo "appdir    Imaging" >> "$file"
        else
	    echo "appdir    Spectroscopy" >> "$file"
        fi

	echo "datadir   ${datadirline}" >> "$file" 
        
    elif [ "x$hasdatadir" != "xy" ]
    then
	setdatadir "$file"
    fi


    # data
    file="$vnmrsystem"/adm/users/profiles/data/$1
    if [ ! -f "$file" ]
    then
	echo "Creating $file"
	if [ "x$osname" = "xInterix" ]
	then
	    echo 'private;'"$user_dir"'/data' > "$file"
	else
	    echo 'private:'"$user_dir"'/data' > $file
	fi
    fi
    
    # templates
    file="$vnmrsystem"/adm/users/profiles/templates/$1
    if [ ! -f "$file" ]
    then
	echo "Creating $file"
        if [ "x$APP_MODE" = "ximaging" ]
        then
	    cat "$vnmrsystem"/imaging/templates/vnmrj/properties/filename_templates > "$file"
	    echo 'RFCOIL:$RFCOIL$_' >> "$file"
	elif [ "x$APP_MODE" = "xwalkup" ]
	then
	    cat "$vnmrsystem"/walkup/templates/vnmrj/properties/filename_templates > "$file"
        else
	    cat "$vnmrsystem"/templates/vnmrj/interface/dataTemplateDefault > "$file"
        fi
    fi

    # Kludge an operatorlist file
    dir="$vnmrsystem"/adm/users/operators
    if test ! -d $dir
    then
        echo Creating "$vnmrsystem"/adm/users/operators
        mkdir -p "$vnmrsystem/adm/users/operators"
    fi

    file="$dir/operatorlist"
    # If a file exists, we want to append to it
    if test ! -e $file
    then
        echo "Creating $file"
        echo "#  Operator;Users;Email;Panel Level;Full Name;Profile Name;" > $file
        echo "$vnmr_adm  $vnmr_adm;null;30;$vnmr_adm;AllLiquids" >> $file
    else
        # Append to the file
         echo "$1  $1;null;30;$1;AllLiquids" >> $file
    fi

    # group
    file="$vnmrsystem"/adm/users/group
    if [ ! -f "$file" ]
    then
	echo "Creating $file"
	echo "vnmr:VNMR group:$1" > "$file"
        echo "agilent and me: Myself and system:me, varian" >> "$file"
    else
	grep $1 "$file"
        if [ $? -ne 0 ]
        then
	    ( cd "$vnmrsystem"/adm/users;
                cat "$file" | sed "s/VNMR group:/VNMR group: $1, /g" > gp.out;
                mv gp.out group
	    )  
        fi
    fi

}

updateadminfiles()
{
    if [ "x$osname" = "xInterix" ]
    then

	# user
	file="$vnmrsystem"/adm/users/profiles/user/$1

	appmode=`/bin/cat "$vnmrsystem"/adm/users/profiles/system/"$1" | awk '/itype/ {print $2}'`
	if [ "x$appmode" = "x" ]
	then
	    appmode=`/bin/cat "$file" | awk '/itype/ {print $2}'`
	fi
	
	# if the file exists, then update sysdir and winappdir to current vnmrsysdir
	if [ -f ""$file"" ]
	then
	    /bin/cat "$file" | sed '/^sysdir/c\
sysdir    '${vnmrsysdir}'
' > ${file}.new
	    /bin/mv ${file}.new "$file"
	    
	    if [ "x$appmode" = "xImaging" ]
	    then
		/bin/cat "$file" | sed '/^winappdir/c\
winappdir    '${user_dir}';'${vnmrsysdir}''${slash}'imaging;'${vnmrsysdir}'
' > ${file}.new
	    elif [ "x$appmode" = "xWalkup" ]
	    then
		/bin/cat "$file" | sed '/^winappdir/c\
winappdir    '${user_dir}';'${vnmrsysdir}''${slash}'walkup;'${vnmrsysdir}'
' > ${file}.new
	    else
		/bin/cat "$file" | sed '/^winappdir/c\
winappdir    '${user_dir}';'${vnmrsysdir}'
' > ${file}.new	
	    fi
            /bin/mv ${file}.new "$file"
	fi
    fi
}

makealladminfiles()
{
    prfiles='system user data templates'
    for file in $prfiles
    do
	if test ! -d "$vnmrsystem/adm/users/profiles/$file"
        then
	    mkdir -p "$vnmrsystem/adm/users/profiles/$file"
        fi
    done

# Only used if DB is being used and a separate DB owner is used
#    if [ "x$osname" = "xInterix" ]
#    then
#	db_adm="postgres"
#	echo "$vnmrdb_home_dir" "$vnmrdbuser_dir"
#	makeadminfiles "$db_adm" "db" "$vnmrdb_home_dir" "$vnmrdbuser_dir"
#    fi

    # This is called for all OS types for the vnmr admin owner
    makeadmfiles "$vnmr_adm" " " "$vnmradm_home_dir" "$vnmruser_dir"
    


}

makechoicefiles()
{
# pis

    if [ "x$APP_MODE" = "x" ]
    then
        file="$vnmradm_home_dir/vnmrsys/global"
        if [ -f "$file" ]
        then
            APP_MODE=`cat "$file" | awk '/appmode/ {getline; print $NF}' | sed 's/"//g'`
        fi
    fi
    if [ "x$APP_MODE" = "ximaging" ]
    then
        file=$vnmrsystem/imaging/templates/vnmrj/choicefiles
    else
        file="$vnmrsystem"/templates/vnmrj/choicefiles
    fi

    if [ ! -f "$file" ]
    then
        echo "Creating $file/pis"
        if test ! -d "$file"
        then
            mkdir -p "$file"
        fi
        echo "\"\" \"\"" > "$file"/pis
        echo "\"$vnmr_adm\" \"$vnmr_adm\"" >> "$file"/pis
    fi
}

########################################################################################
# main, Main, MAIN
########################################################################################
username=$1
userhome=$2
vnmradm_home_dir=$2
osname=`uname -s`


if [ "x$osname" = "xInterix" ]
then
    vnmruser_dir="${userhome}"\\\\vnmrsys
#    vnmrdbuser_dir="${vnmrdb_home_dir}"\\\\vnmrsys
else
    vnmruser_dir=${userhome}/vnmrsys
fi 

if [ "x$vnmruser" = "x" ]
then
    vnmruser="${vnmruser_dir}"
    export vnmruser
fi

sfu_ver=`uname -r`
if [ "x$osname" = "xInterix" -a "x$sfu_ver" = "x3.5" ]
then
  # hardcoding XP for now ....    GMB
  vnmradm_home_dir="C:/SFU/home/$username"
  vnmruser="/home/$username/vnmrsys"
  vnmruser_dir="C:/SFU/home/$username/vnmrsys"
fi

export vnmruser
export vnmradm_home_dir
export vnmruser_dir

echo "user: $username"
echo "vnmruser: $vnmruser"
echo "vnmradm_home_dir: $vnmradm_home_dir"
echo "vnmruser_dir: $vnmruser_dir"

# Create directories as needed
prfiles='system user data templates'
for file in $prfiles
do
    if test ! -d "$vnmrsystem/adm/users/profiles/$file"
    then
	mkdir -p "$vnmrsystem/adm/users/profiles/$file"
    fi
done

# Only used if DB is being used and a separate DB owner is used
#    if [ x"$osname" = "xInterix" ]
#    then
#	db_adm="postgres"
#	echo "$vnmrdb_home_dir" "$vnmrdbuser_dir"
#	makeadminfiles "$db_adm" "db" "$vnmrdb_home_dir" "$vnmrdbuser_dir"
#    fi

#makechoicefiles 

makeadmfiles $username " " "$vnmradm_home_dir" "$vnmruser_dir"

#updateadminfiles $username "$vnmradm_home_dir"


