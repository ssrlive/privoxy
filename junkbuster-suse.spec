# $Id: junkbuster-suse.spec,v 1.13 2002/03/07 18:25:56 swa Exp $
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
# Revision 1.13  2002/03/07 18:25:56  swa
# synced redhat and suse build process
#
# Revision 1.12  2002/03/02 15:50:04  swa
# 2.9.11 version. more input for docs.
#
# Revision 1.11  2001/12/02 10:29:26  swa
# New version made these changes necessary.
#
# Revision 1.10  2001/10/31 19:27:27  swa
# consistent description. new name for suse since
# we had troubles with rpms of identical names
# on the webserver.
#
# Revision 1.9  2001/10/26 18:17:23  swa
# new version string
#
# Revision 1.8  2001/09/13 16:22:42  swa
# man page is legacy. suse rpm now contains html
# documentation.
#
# Revision 1.7  2001/09/10 17:44:22  swa
# integrate three pieces of documentation.
#
# Revision 1.6  2001/09/10 16:29:23  swa
# binary contained debug info.
# buildroot definition fucks up the build process under suse.
# program needs to write in varlogjunkbuster
# install all templates
# create varlogjunkbuster
#
# Revision 1.5  2001/06/09 09:13:29  swa
# description shorter
#
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

%define ijbconf %{_sysconfdir}/%{name}

Summary:      The Internet Junkbuster
Vendor:       http://ijbswa.sourceforge.net
Name:         junkbuster-suse
Distribution: defineme
Version: 2.9.11
Release: 1
Source: http://www.waldherr.org/%{name}/ijbswa-%{version}.tar.gz
# not sure if this works
BuildRoot: %{_tmppath}/%{name}-%{version}-root
Packager:     Stefan Waldherr <stefan@waldherr.org>
Copyright:    GPL
Group:        Networking/Utilities
URL: http://ijbswa.sourceforge.net/
Provides:     ijb
Obsoletes:    ijb
Autoreqprov:  on

#
# -----------------------------------------------------------------------------
#
%description
Internet Junkbuster is a web proxy with advanced filtering
capabilities for protecting privacy, filtering web page content,
managing cookies, controlling access, and removing ads, banners,
pop-ups and other obnoxious Internet Junk. Junkbuster has a very
flexible configuration and can be customized to suit individual needs
and tastes. Internet Junkbuster has application for both stand-alone
systems and multi-user networks.

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
autoheader
autoconf
./configure
make
make dok
strip junkbuster

#
# -----------------------------------------------------------------------------
# hint by kukuk@suse.de
%pre
usr/sbin/groupadd -r junkbuster
usr/sbin/useradd -g junkbuster -d /etc/junkbuster -r junkbuster -s "/bin/false" > /dev/null 2>&1 || /bin/true

#
# -----------------------------------------------------------------------------
#
%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
mkdir -p ${RPM_BUILD_ROOT}%{_sbindir} \
         ${RPM_BUILD_ROOT}%{_mandir}/man8 \
         ${RPM_BUILD_ROOT}/var/log/junkbuster \
         ${RPM_BUILD_ROOT}%{ijbconf}/templates \
         ${RPM_BUILD_ROOT}%{_sysconfdir}/logrotate.d \
         ${RPM_BUILD_ROOT}%{_sysconfdir}/init.d
gzip README AUTHORS ChangeLog junkbuster.1 || /bin/true
install -s -m 744 junkbuster $RPM_BUILD_ROOT%{_sbindir}/junkbuster
cp -f junkbuster.1.gz $RPM_BUILD_ROOT%{_mandir}/man8/junkbuster.8.gz
cp -f *.action $RPM_BUILD_ROOT%{ijbconf}/
cp -f re_filterfile $RPM_BUILD_ROOT%{ijbconf}/re_filterfile
cp -f trust $RPM_BUILD_ROOT%{ijbconf}/trust
cp -f templates/*  $RPM_BUILD_ROOT%{ijbconf}/templates/
cp -f junkbuster.logrotate $RPM_BUILD_ROOT%{_sysconfdir}/logrotate.d/junkbuster
install -m 755 junkbuster.init.suse $RPM_BUILD_ROOT%{_sysconfdir}/init.d/junkbuster
install -m 711 -d $RPM_BUILD_ROOT/var/log/junkbuster
ln -sf /etc/init.d/junkbuster $RPM_BUILD_ROOT/usr/sbin/rcjunkbuster

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
    $RPM_BUILD_ROOT%{ijbconf}/config

#
# -----------------------------------------------------------------------------
#
%post
# for upgrade from 2.0.x
if [ -f /var/log/junkbuster/junkbuster ]; then
 mv -f /var/log/junkbuster/junkbuster /var/log/junkbuster/logfile
 chown -R junkbuster:junkbuster /var/log/junkbuster 2>/dev/null
 chown -R junkbuster:junkbuster /etc/junkbuster 2>/dev/null
fi
sbin/insserv etc/init.d/junkbuster

#
# -----------------------------------------------------------------------------
#
%postun
sbin/insserv etc/init.d/

#
# -----------------------------------------------------------------------------
#
%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

#
# -----------------------------------------------------------------------------
#
%files
%defattr(-,root,root)
%doc README.gz AUTHORS.gz ChangeLog.gz
%doc doc/webserver/developer-manual doc/webserver/user-manual
%doc doc/webserver/user-manual
#%doc junkbuster.weekly junkbuster.monthly AUTHORS
%dir %{ijbconf}
%config %{ijbconf}/*
%attr(0744,junkbuster,junkbuster) %dir /var/log/junkbuster
%config %{_sysconfdir}/logrotate.d/junkbuster
%attr(0755,root,root)/usr/sbin/junkbuster
%{_mandir}/man8/*
%config %{_sysconfdir}/init.d/junkbuster
/usr/sbin/rcjunkbuster

#
# -----------------------------------------------------------------------------
#
%changelog
* Thu Mar  7 2002 Stefan Waldherr <stefan@waldherr.org>
- major rework of rpm, help by kukuk@suse.de

* Sun Mar 03 2002 Hal Burgiss <hal@foobox.net>
- /bin/false for shell causes init script to fail. Reverting.

* Wed Jan 09 2002 Hal Burgiss <hal@foobox.net>
- Removed UID 73. Included user-manual and developer-manual in docs.
  Include other actions files. Default shell is now /bin/false.
  Userdel user=junkbust. ChangeLog was not zipped. Removed
  RPM_OPT_FLAGS kludge.

* Fri Dec 28 2001 Thomas Steudten <thomas@steudten.ch>
- add paranoia check for 'rm -rf $RPM_BUILD_ROOT'
- add gzip to 'BuildRequires'

* Sat Dec  1 2001 Hal Burgiss <hal@foobox.net>
- actionsfile is now ijb.action.

* Tue Nov  6 2001 Thomas Steudten <thomas@steudten.ch>
- Compress manpage
- Add more documents for installation
- Add version string to name and source

* Wed Oct 24 2001 Hal Burigss <hal@foobox.net>
- Back to user 'junkbuster' and fix configure macro.

* Wed Oct 10 2001 Hal Burigss <hal@foobox.net>
- More changes for user 'junkbust'. Init script had 'junkbuster'.

* Sun Sep 23 2001 Hal Burgiss <hal@foobox.net>
- Change of $RPM_OPT_FLAGS handling. Added new HTML doc files.
- Changed owner of /etc/junkbuster to shut up PAM/xauth log noise.

* Thu Sep 13 2001 Hal Burgiss <hal@foobox.net>
- Added $RPM_OPT_FLAGS support, renaming of old logfile, and
- made sure no default shell exists for user junkbust.

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
