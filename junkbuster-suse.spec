# $Id: junkbuster-suse.spec,v 1.4 2001/06/08 20:53:36 swa Exp $
#
# Written by and Copyright (C) 2001 the SourceForge
# IJBSWA team.  http://ijbswa.sourceforge.net
#
# Based on the Internet Junkbuster originally written
# by and Copyright (C) 1997 Anonymous Coders and 
# Junkbusters Corporation.  http://www.junkbusters.com
#
# This program is free software; you can redistribute it 
# and/or modify it under the terms of the GNU General
# Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at
# your option) any later version.
#
# This program is distributed in the hope that it will
# be useful, but WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.  See the GNU General Public
# License for more details.
#
# The GNU General Public License should be included with
# this file.  If not, you can view it at
# http://www.gnu.org/copyleft/gpl.html
# or write to the Free Software Foundation, Inc., 59
# Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# $Log: junkbuster-suse.spec,v $
# Revision 1.4  2001/06/08 20:53:36  swa
# use buildroot, export init to separate file (better manageability)
#
# Revision 1.3  2001/06/07 17:28:10  swa
# cosmetics
#
# Revision 1.2  2001/06/07 17:18:44  swa
# header fixed
#
#
# neededforbuild  -ijb
# usedforbuild    -ijb aaa_base aaa_dir autoconf automake base bash bindutil binutils bison bzip compress cpio cracklib db devs diffutils e2fsprogs file fileutils findutils flex gawk gcc gdbm gdbm-devel gettext glibc glibc-devel gpm gppshare groff gzip kbd less libtool libz lx_suse make mktemp modutils ncurses ncurses-devel net-tools netcfg nkitb pam pam-devel patch perl pgp ps rcs rpm sendmail sh-utils shadow strace syslogd sysvinit texinfo textutils timezone unzip util-linux vim xdevel xf86 xshared

Vendor:       http://ijbswa.sourceforge.net
Distribution: defineme
Name:         junkbuster
Packager:     Stefan Waldherr <stefan@waldherr.org>

Copyright:    GPL
BuildRoot: /tmp/junkbuster-rpmbuild
Group:        Networking/Utilities
Provides:     ijb
Obsoletes:    ijb
Autoreqprov:  on
Version: 2.9
Release: 4
Summary:      The Internet Junkbuster
Source:  http://www.waldherr.org/junkbuster/ijbswa.tar.gz

#
# -----------------------------------------------------------------------------
#
%description
The Internet Junkbuster stops your browser from displaying the
advertisement images that pervade many commercial web pages.  Since
your browser has to download fewer images, surfing the web should be
faster.

Authors:
--------
    http://ijbswa.sourceforge.net

SuSE series: n

#
# -----------------------------------------------------------------------------
#
%prep
%setup -c -n ijbswa

#
# -----------------------------------------------------------------------------
#
%build
./configure
make

#
# -----------------------------------------------------------------------------
#
%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/{var/log/junkbuster,usr/{sbin,share/man/man1},etc/{junkbuster,junkbuster/templates,init.d}}

install -m 755 junkbuster.init.suse $RPM_BUILD_ROOT/etc/init.d/junkbuster
ln -sf $RPM_BUILD_ROOT/etc/init.d/junkbuster $RPM_BUILD_ROOT/usr/sbin/rcjunkbuster

install -m 755 junkbuster $RPM_BUILD_ROOT/usr/sbin
install -d $RPM_BUILD_ROOT/etc/junkbuster
install -d $RPM_BUILD_ROOT/etc/junkbuster/templates
install -m 644 permissionsfile $RPM_BUILD_ROOT/etc/junkbuster
install -m 644 re_filterfile $RPM_BUILD_ROOT/etc/junkbuster
# verify all file locations, etc. in the config file
# don't start with ^ or commented lines are not replaced
cat config | \
    sed 's/^confdir.*/confdir \/etc\/junkbuster/g' | \
#    sed 's/^permissionsfile.*/permissionsfile \/etc\/junkbuster\/permissionsfile/g' | \
#    sed 's/^re_filterfile.*/re_filterfile \/etc\/junkbuster\/re_filterfile/g' | \
#    sed 's/^logfile.*/logfile \/var\/log\/junkbuster\/logfile/g' | \
#    sed 's/^jarfile.*/jarfile \/var\/log\/junkbuster\/jarfile/g' | \
#    sed 's/^forward.*/forward \/etc\/junkbuster\/forward/g' | \
#    sed 's/^aclfile.*/aclfile \/etc\/junkbuster\/aclfile/g' > \
    sed 's/^logdir.*/logdir \/var\/log\/junkbuster/g' > \
    config.tmp
cp -f config.tmp config
install -m 644 config $RPM_BUILD_ROOT/etc/junkbuster
#install -m 644 forward $RPM_BUILD_ROOT/etc/junkbuster
install -m 644 trust $RPM_BUILD_ROOT/etc/junkbuster
install -m 644 templates/default $RPM_BUILD_ROOT/etc/junkbuster/templates
install -m 644 templates/show-status $RPM_BUILD_ROOT/etc/junkbuster/templates
install -m 644 templates/show-status-file $RPM_BUILD_ROOT/etc/junkbuster/templates
install -m 644 junkbuster.1 $RPM_BUILD_ROOT/usr/share/man/man1
%{?suse_check}

#
# -----------------------------------------------------------------------------
#
%post
sbin/insserv etc/init.d/junkbuster

#
# -----------------------------------------------------------------------------
#
%postun
sbin/insserv etc/init.d/

#
# -----------------------------------------------------------------------------
#
%files
#%doc README *.html
/usr/sbin/junkbuster
/usr/share/man/man1/junkbuster.1.gz
%config(noreplace) /etc/junkbuster
/etc/init.d/junkbuster
/usr/sbin/rcjunkbuster

#
# -----------------------------------------------------------------------------
#
%changelog -n junkbuster
* Sun Jun  3 2001 Stefan Waldherr <stefan@waldherr.org>
- rework of RPM
* Wed Feb 14 2001 - uli@suse.de
- fixed init script
* Wed Dec 06 2000 - bjacke@suse.de
- renamed package to junkbuster
- fixed copyright tag
* Thu Nov 30 2000 - uli@suse.de
- moved init script to /etc/init.d
* Wed Feb 16 2000 - kukuk@suse.de
- Move /usr/man -> /usr/share/man
- Mark /etc/ijb as "config(noreplace)"
* Mon Sep 20 1999 - uli@suse.de
- fixed init script
* Mon Sep 13 1999 - bs@suse.de
- ran old prepare_spec on spec file to switch to new prepare_spec.
* Thu Apr 01 1999 - daniel@suse.de
- do not start ijb as root (security)
* Tue Mar 30 1999 - daniel@suse.de
- don´t use saclfile.ini
* Tue Mar 30 1999 - daniel@suse.de
- small fix to whitelist-configuration,
  version is and was 2.0.2 WITHOUT Stefan Waldherr's patches
  (http://www.waldherr.org/junkbuster/)
* Mon Mar 01 1999 - daniel@suse.de
- new package: version 2.0
