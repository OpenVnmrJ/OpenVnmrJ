# OpenVnmrJ

OpenVnmrJ is free and open-source software owned by the University of Oregon. Read more about OpenVnmrJ and it's history at [Background](http://openvnmrj.org/Background).

OpenVnmrJ Version 3 is the latest OpenVnmrJ release. It contains additions as described in the [Notes.txt](Notes.txt) file. Installation instructions are in the [Install](Install.md) file. OpenVnmrJ Version 3 can co-exist with any VnmrJ version. However, VnmrJ need not be installed in order for OpenVnmrJ to be completely functional as a spectrometer.
OpenVnmrJ 3 supports the following:
- CentOS:    Version 7.5 - 7.9.
- RHEL:      Version 8.4
- AlmaLinux: Version 8.4
- Ubuntu:    Versions 20.
- MacOS:     Versions 10.13 (High Sierra) to 11.3 (Big Sur). (data station only)

OpenVnmrJ Version 2 was the first OpenVnmrJ release to not require VnmrJ 4.2 to be installed in order to function as a spectrometer host. It contains additions as described in the [Notes.txt](Notes.txt) file. Installation instructions are in the [Install2](Install2.md) file.
OpenVnmrJ 2 supports the following:
- CentOS:    Versions 6, 7, and 8.
- RHEL:      Versions 6, 7, and 8.
- Ubuntu:    Versions 14, 16, 18, and 20. (data station only)
- MacOS:     Versions 10.10 (Yosemite) to 11.3 (Big Sur). (data station only)


OpenVnmrJ Version 1 was the original release of the open-sourced parts of Varian and Agilent's VnmrJ 4.2.  For a completely functional spectrometer, it required that VnmrJ 4.2 also be installed.

If you want to use or test OpenVnmrJ, download the release appropriate for your OS, see below.  

## Releases
[![Github All Releases](https://img.shields.io/github/downloads/OpenVnmrJ/OpenVnmrJ/total.svg?maxAge=2592000?style=flat-square)]()  

The latest binary releases for OpenVnmrJ are available from [Releases](https://github.com/OpenVnmrJ/OpenVnmrJ/releases).  

<a href="https://doi.org/10.5281/zenodo.4304999"><img src="https://zenodo.org/badge/DOI/10.5281/zenodo.4304999.svg" alt="DOI"></a>


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
  

## Building

Current Github Action status: [![Build and Test OpenVnmrJ](https://github.com/OpenVnmrJ/OpenVnmrJ/actions/workflows/main.yml/badge.svg)](https://github.com/OpenVnmrJ/OpenVnmrJ/actions/workflows/main.yml)


If you are interested in building OpenVnmrJ from source, refer to the
[ovjTools repository](https://github.com/OpenVnmrJ/ovjTools), which
contains instructions and all the tools and libraries necessary for
building OpenVnmrJ.

## LICENSE

This work is licensed under the Apachev2 license.  

However, optionally linking to the Gnu Scientific Library (GSL) or to other GPLv3 components, will cause the deriviative work to be licensed under the GPLv3.  
