
# a version for Solaris

alias lf ls -F
alias ll ls -l
alias vnmrx vnmr o
umask 022

#  the who program with "am i" as arguments does
#  not like to be run on a non-terminal-based shell

if (! $?term) exit

set history=300
alias h history
set whoami=`id | tr '()' '  ' | cut -f2 -d' '`
set prompt="`uname -n`:$whoami \!>"
