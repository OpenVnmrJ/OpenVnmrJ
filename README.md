# OpenVnmrJ

OpenVnmrJ is the open-sourced parts of Varian and Agilent's VnmrJ 4.2, now owned by the University of Oregon and is free and open-source software.
Read more about OpenVnmrJ and the history of VnmrJ at [Background](http://openvnmrj.org/Background).  

OpenVnmrJ does not contain console software, example FIDs, NMRPipe or ChemPack, however during installation, these will be copied from your existing
VnmrJ 4.2 into the OpenVnmrJ directory. The /vnmr symlink will be changed to point to OpenVnmrJ and your existing VnmrJ 4.2 installation will be untouched.

Never delete the Agilent supplied VnmrJ 4.2 on a spectrometer computer—Agilent service will require the original VnmrJ 4.2.

If you want to use or test OpenVnmrJ, download the release appropriate for your OS, see below.  

## Releases

The latest binary releases for OpenVnmrJ are available from [Releases](https://github.com/OpenVnmrJ/OpenVnmrJ/releases).  
There are releases for:  
- RHEL/CentOS: Built on CentOS 6.3, tested on 6.3 and 6.7
- Ubuntu: Built and tested on Trusty Tahr (14.04)
- OS X: Built on 10.11 (El Capitan) and tested on 10.11 and 10.10 (Yosemite)

## Read more

For more information on the OpenVnmrJ project, visit [openvnmrj.org](http://openvnmrj.org).

## Help us

We are looking for anyone to help with this projects. Coding is not necessary! Translations, pulse sequences, macros, beta testing and bug reporting are
very helpful. See [Contributing](http://openvnmrj.org/Contributing/) for more information.  

## Contribute Appdirs

We welcome pulse sequences, libraries of macros, UX improvments contributed as an Appdir. Check out the [OpenVnmrJ Appdirs](https://github.com/OpenVnmrJ/appdirs) 
repository on how to download and install user contributed appdirs and how to contribute your appdir. 
Feel free to use [Slack](https://openvnmrj.slack.com/messages/appdirs/) if you need some help.  

## Building

If you are interested in building OpenVnmrJ from source, refer to the [ovjTools repository](https://github.com/OpenVnmrJ/ovjTools), which contains instructions and
all the tools and libraries necessary for building OpenVnmrJ.  

## LICENSE

This work is licensed under the Apachev2 license.  

However, optionally linking to the Gnu Scientific Library (GSL) or to other GPLv3 components, will cause the deriviative work to be licensed under the GPLv3.  
