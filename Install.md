
OpenVnmrJ Version 3.1 Installation or Upgrade

For upgrade information, see the end of this document.

OpenVnmrJ Version 3 has been installed on CentOS systems running
versions 7.5, 7.6, 7.7, 7.8, 7.9, and RHEL systems running 8.3 and 8.4.
It has also been installed on AlmaLinux 8.4 and on Ubuntu
Version 18 and 20 systems.

The MacOS version of OpenVnmrJ has been installed on versions Yosemite (10.10)
to Big Sur (16.3). Installation on RHEL/CentOS and Ubuntu 20 systems
can be used to run a spectrometer. The MacOS version is for data-processing only.

To install on CentOS 6.x systems, we recommend using the CentOS KickStart
DVD available from Agilent, available [here](https://drive.google.com/drive/folders/10-pQ-HquslWJfWBkoOj7cM0yHST5RFbU)
Note that support for CentOS 6 from RedHat has ended and OpenVnmrJ 3.1
does not support CentOS 6.x. OpenVnmrJ 2.1 does support CentOS 6.x.
Also, if one uses the ovjBuild script from OpenVnmrJ 3.1 to build a release
on CentOS 6 directly from the github source code, it will function of that platform.

For CentOS 7.x and 8.x systems, one can start by installing a standard
OS using the CentOS-7-x86_64-DVD-*.iso or CentOS-8-x86_64-DVD-*.iso images
available from https://www.centos.org/download/
Note that support for CentOS 8 from RedHat will end at the end of 2021.

RedHat has introduced a no cost developer subscription for up to 
16 systems. [Details are here](https://developers.redhat.com/blog/2021/02/10/how-to-activate-your-no-cost-red-hat-enterprise-linux-subscription/?sc_cid=7013a0000026TeTAAU)

For AlmaLinux systems, one can start by downloading one of the x86_64 images
available from https://mirrors.almalinux.org/isos

For Ubuntu systems, one can start by downloading one of the images available
from https://ubuntu.com/download/desktop

If earlier versions of OS, for example, CentOS_6.9, Centos_7.6, or
Ubuntu_16, were chosen, then the system, if connected to network, will
prompt for OS updates at a later stage.  It is up to the user and/or
their organization policies to opt for OS updates or not.

A typical Linux 7.x or 8.x installation may involve selecting, in the
"SOFTWARE SELECTION" options, the GNOME Desktop as the base environment
and the GNOME applications, Legacy X Window System Compatibility,
Compatibility Libraries, and Development Tools as add-ons. One does
not need to set a hostname or connect to a network at this time.
After clicking "Begin Installation", set the root password and click
the "USER CREATION" button to make vnmr1, which will be the default
OpenVnmrJ administrator.

Following installation and a reboot, login in as root, accept the license,
turn on the network and click the "Finish configuration" button. After a
few more configuration items, it is ready for installation of OpenVnmrJ.

The OpenVnmrJ installer may be downloaded from
  https://github.com/OpenVnmrJ/OpenVnmrJ/releases
Move the zip file to /tmp and unzip it.  If you are logged in
as root, do not try to install it from the root home directory (/root). It will
not work. First unzip the file and cd into dvdimageOVJ (or dvdimageOVJMI).
Run ./load.nmr.  The first thing it will do is install missing CentOS
packages. It uses yum and will require network access. Since it turns off
SELinux, a reboot is needed after the packages are installed.

After reboot, log in as root
again and cd to the dvdimageOVJ directory and run ./load.nmr again. It will
find that all required packages have been installed and start a normal
OpenVnmrJ installation. Following that, it will prompt you to set up
some standard accounts (walkup and service), install the latest version
of NMRPipe, install the VnmrJ 4.2 manual set, and setup the network for
console communications. If you make the walkup and service accounts, on
Ubuntu systems, they will be given the initial password of abcd1234.
On CentOS, RHEL, and AlmaLinux systems, they will not be given a password.

Note that access to the network is tested by trying to "ping" google.com 
Some firewalls disable ping. If you are sure you have network access,
one can bypass the "ping" test by using
   ./load.nmr noPing

Note that the default OS package installation downloads them from the
network.  To support installation of OpenVnmrJ without network access, two
new scripts have been implemented. They are in the dvdimageOVJ directory
along with load.nmr. The ovjGetRepo command is a link to the installpkgs
script. Network access is required for this command. It will download
and save the required packages and their dependencies. It will also
install them. For this to work, a system with a minimum of packages is
needed, since if the package is already installed, the ovjGetRepo script
will not download it.  To create the repositories described below, a fresh
install of CentOS 7 with only the Gnome desktop and "Development tools"
selection was used. The ovjGetRepo script puts the packages in a directory
named openvnmrj.repo in the parent directory of the dvdimageOVJ direcory.
For the repositories described below, this was repeated for CentOS 7.9
and AlmaLinux 8.4 since each has different package dependencies.  The
openvnmrj.repo directories were zipped and given names to reflect the
specific OS.  They can be downloaded with these links:

   https://www.dropbox.com/s/yi5s29olowogia8/ovj3Centos79.repo.zip?dl=0

   https://www.dropbox.com/s/hsav3k3flowkhxn/ovj3AlmaLinux84.repo.zip?dl=0

The second script is ovjUseRepo. It takes an optional path name for the
openvnmrj.repo but defaults to the parent of the dvdimageOVJ directory.
This script makes a repository and adds it to the list of yum repos.
If the installpkgs script, called by load.nmr, detects openvnmrj.repo,
it will not do the ping test and it disables all other repositories.
Internet access is not required. A typical process would be to download
the OpenVnmrJ installer and one of the above CentOS repositories. Move
both files to /tmp and then do
   unzip <OpenVnmrJ installer>
   unzip <OS package repository>
   cd dvdimageOVJ
   ./ovjUseRepo
   ./load.nmr

Installations on Ubuntu systems are similar to those for CentOS.
One difference is that the user account created during the installation
of Ubuntu will be given "admin" privileges. That is, by using sudo,
that account can do anything the root account on CentOS can do.
Some may prefer that the vnmr1 account does not have those privileges.
One could then create an initial account, such as ovjroot.  During the
installation of OpenVnmrJ, the vnmr1 account will be created with no
admin privileges. It will be given the initial password of abcd1234.

After the OpenVnmrJ installation is complete, log out and log in before using
the administrator (vnmr1) account.

Newer CentOS and Ubuntu systems use the Gnome 3 display manager.
The "Standard" display manager no longer has desktop icons. Rather, it has
an Activities button at the upper left corner. Clicking that button will
display a "Type to search..." entry field. If you enter vnmrj or openvnmr,
the OpenVnmrJ launch icon will appear. This can be dragged to the favorites
toolbar or right-clicked and select "Add to Favorites". If the OpenVnmrJ
launch icon does not appear, re-run makeuser on that account. You may also
need to log out and log in again.

Gnome 3 also supports the "Classic" display manager, where desktop icons
are displayed. One can select the display manager from the login screen.
A settings icon is available after selecting the user, but before entering
the password. On CentOS, it is next to the "Sign in" button. Select the
"Classic (Wayland display server)" and then log in. On Ubuntu, the settings
icon is at the lower right-hand corner of the display. Select either of
the "Ubuntu" settings. The OpenVnmrJ desktop launch icons will be displayed.
Before they can be used, they must be enabled by clicking the right mouse
button on them and selecting "Allow Launching".


The MacOS version needs java version 1.8 or newer to be installed.
To install on the Mac, open the "Finder" and navigate to the OpenVnmrj.pkg
file. Right-click on the icon and select "Open".
If you double-click on the icon, it may result in System security
complaining it’s “not registered developer” and then quit.
To get around this security issue, go to
  Apple Pull down menu
    System Preferences
    Security & Privacy 
    Click on lock icon bottom left of window and enter administrator password
        General Tab
        Bottom Section
        Allow apps downloaded from:
           Select OpenVnmrJ and choose “Open Anyways”

Also, if installed on MacOS Catalina (10.15) or Big Sur (16.3), the
Mac must be rebooted following installation of OpenVnmrJ. If one
installs OpenVnmrJ Version 3 on a Mac running Mojave or older versions and
subsequently upgrades to Catalina or newer MacOS versions, OpenVnmrJ
will stop working.  This is because Catalina and newer MacOS versions
have implemented a "read-only" system directory and OpenVnmrJ will have
been moved to a different disk locations and the /vnmr link will have
been removed. To re-enable OpenVnmrJ, run the script

  ~/vnmrsys/vnmr/bin/ovjFixMac

A system reboot will then be necessary.

The open-source version of java for MacOS may be obtained from https://jdk.java.net.
Download, for example, the JDK 14.0.2 release. Unpack the java package with the commands

  gunzip openjdk-14.0.2_osx-x64_bin.tar.gz

  tar xvf openjdk-14.0.2_osx-x64_bin.tar

Then move the jdk-14.0.2.jdk directory with the command

  sudo mv jdk-14.0.2.jdk /Library/Java/JavaVirtualMachines


Upgrading an existing OpenVnmrJ installation.
=============================================

Running
  ./load.nmr
will install the new release as described above. If one instead runs
  ./upgrade.nmr
then OpenVnmrJ will be upgraded in place. That is, a new directory to hold
the release will not be created. The current system and user configuration
files will be maintained, along with any gradient shimming, probe, or
other calibration files. All other files that have changed or been added
since the previous version will be installed. There is no graphical
interface when the upgrade is performed.
