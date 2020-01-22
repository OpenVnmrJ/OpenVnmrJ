# OpenVnmrJ

OpenVnmrJ is free and open-source software owned by the University of Oregon. Read more about OpenVnmrJ and it's history at [Background](http://openvnmrj.org/Background).

OpenVnmrJ Version 2 is the latest OpenVnmrJ release. It contains additions as described in the [Notes.txt](Notes.txt) file. Installation instructions are in the [Install](Install.md) file. OpenVnmrJ Version 2 can co-exist with any VnmrJ version. However, VnmrJ need not be installed in order for OpenVnmrJ to be completely functional as a spectrometer.

OpenVnmrJ Version 1 was the original release of the open-sourced parts of Varian and Agilent's VnmrJ 4.2.  For a completely functional spectrometer, it required that VnmrJ 4.2 also be installed.

If you want to use or test OpenVnmrJ, download the release appropriate for your OS, see below.  

## Releases
[![Github All Releases](https://img.shields.io/github/downloads/OpenVnmrJ/OpenVnmrJ/total.svg?maxAge=2592000?style=flat-square)]()  

The latest binary releases for OpenVnmrJ are available from [Releases](https://github.com/OpenVnmrJ/OpenVnmrJ/releases).  
There are releases for:
- RHEL/CentOS: Versions 6 and 7.
- Ubuntu: Versions 14, 16, and 18.
- MacOS: Versions 10.10 (Yosemite) to 10.15 (Catalina).

## Read more

For more information on the OpenVnmrJ project, visit [openvnmrj.org](http://openvnmrj.org).

## Help us

We are looking for anyone to help with this projects. Coding is not necessary! Translations, pulse sequences, macros, beta testing and bug reporting are
very helpful. See [Contributing](http://openvnmrj.org/Contributing/) for more information.  

## Contributors
 * Timothy Burrow
 * Dan Iverson
 * Gareth Morris  
 * Michael Tesch

 
## Contribution to Appdirs

We welcome pulse sequences, libraries of macros, UX improvments contributed as an Appdir. Check out the [OpenVnmrJ Appdirs](https://github.com/OpenVnmrJ/appdirs) 
repository on how to download and install user contributed appdirs and how to contribute your appdir. 

In OpenVnmrJ, type `ovjapps` to list user contributed application directories.

Feel free to use [Slack](https://openvnmrj.slack.com/messages/appdirs/) if you need some help.  

## Building

Current Travis-CI build status:
[![Travis-CI Build](https://travis-ci.org/tesch1/OpenVnmrJ.svg?branch=master)](https://travis-ci.org/tesch1/OpenVnmrJ)

Current Circle CI build status:
[![Circle CI Build](https://circleci.com/gh/tesch1/OpenVnmrJ.svg?&style=shield&circle-token=43b262352b794300ba603dafbf6fc054e828e8b3)](https://circleci.com/gh/tesch1/OpenVnmrJ)

If you are interested in building OpenVnmrJ from source, refer to the
[ovjTools repository](https://github.com/OpenVnmrJ/ovjTools), which
contains instructions and all the tools and libraries necessary for
building OpenVnmrJ.

## LICENSE

This work is licensed under the Apachev2 license.  

However, optionally linking to the Gnu Scientific Library (GSL) or to other GPLv3 components, will cause the deriviative work to be licensed under the GPLv3.  
