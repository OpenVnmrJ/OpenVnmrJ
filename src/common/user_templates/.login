#
#  A version for Linux
#
#
set mail=(/var/mail/$user)
set history=300
alias h history
alias la ls -a
alias lf ls -F
alias ll ls -l

set path=($path /usr/sbin $HOME/bin /etc)

if ( -e .vnmrsilent ) then
    rm .vnmrsilent
else if ( ${?SSH_TTY} ) then
    echo -n 'Input display server name: '
    set server=($<)
    setenv DISPLAY $server":0.0"
endif

setenv graphics sun
set term=xterm

set username=`id | tr '()' '  ' | cut -f2 -d' '`
set prompt="$username \!>"

echo -n "]2;`uname -n`~$username :$cwd"
alias cd 'cd \!*;echo -n "]2;`uname -n`~$username : $cwd"'

if ( -e $HOME/.vnmrenv ) then
    source $HOME/.vnmrenv
endif
