# $Id: junkbuster-rh.spec,v 1.39 2002/03/08 18:57:29 swa Exp $
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

# Defines should happen in the begining of the file
%define ijbconf %{_sysconfdir}/%{name}

Summary: The Internet Junkbuster
Vendor: http://ijbswa.sourceforge.net
Name: junkbuster
Version: 2.9.11
Release: 8
Source0: http://www.waldherr.org/%{name}/ijbswa-%{version}.tar.gz
License: GPL
BuildRoot: %{_tmppath}/%{name}-%{version}-root
Group: Networking/Utilities
URL: http://ijbswa.sourceforge.net/
Obsoletes: junkbuster-raw junkbuster-blank
# Prereq: /usr/sbin/useradd , /sbin/chkconfig , /sbin/service 
Prereq: shadow-utils, chkconfig, initscripts, sh-utils
BuildRequires: perl gzip sed docbook-utils libtool
Conflicts: junkbuster-raw junkbuster-blank

%description
Internet Junkbuster is a web proxy with advanced filtering
capabilities for protecting privacy, filtering web page content,
managing cookies, controlling access, and removing ads, banners,
pop-ups and other obnoxious Internet Junk. Junkbuster has a very
flexible configuration and can be customized to suit individual needs
and tastes. Internet Junkbuster has application for both stand-alone
systems and multi-user networks.

%prep
%setup -q -c -n ijbswa

%build
%configure
make 
make redhat-dok

## Explicitily stripping is not recomended.
## This is handled altomaticaly by RPM, and can couse troubles if
## anyone wants to build an unstriped version - morcego
#strip %{name}

%install
[ "%{buildroot}" != "/" ] && rm -rf %{buildroot}
mkdir -p %{buildroot}%{_sbindir} \
         %{buildroot}%{_mandir}/man8 \
         %{buildroot}%{_localstatedir}/log/%{name} \
         %{buildroot}%{ijbconf}/templates \
         %{buildroot}%{_sysconfdir}/logrotate.d \
         %{buildroot}%{_sysconfdir}/rc.d/init.d 

## Manual gziping of manpages should not be done, once it can
## break the building on some distributions. Anyway, rpm does it
## automagicaly these days
## Gziping the documentation files is not recomended - morcego
#gzip README AUTHORS ChangeLog %{name}.1 || /bin/true

install -s -m 744 %{name} %{buildroot}%{_sbindir}/%{name}

## We need to change the man section internaly on the manpage
## -- morcego (sugestion by Hal Burgiss)
#cp -f %{name}.1 %{buildroot}%{_mandir}/man8/%{name}.8
sed -e 's@^.TH JUNKBUSTER 1@.TH JUNKBUSTER 8@g' %{name}.1 > %{buildroot}%{_mandir}/man8/%{name}.8
cp -f *.action %{buildroot}%{ijbconf}/
cp -f re_filterfile %{buildroot}%{ijbconf}/re_filterfile
cp -f trust %{buildroot}%{ijbconf}/trust
cp -f templates/*  %{buildroot}%{ijbconf}/templates/
cp -f %{name}.logrotate %{buildroot}%{_sysconfdir}/logrotate.d/%{name}
install -m 755 %{name}.init %{buildroot}%{_sysconfdir}/rc.d/init.d/%{name}
install -m 711 -d %{buildroot}%{_localstatedir}/log/%{name}

# verify all file locations, etc. in the config file
# don't start with ^ or commented lines are not replaced
## Changing the sed paramter delimiter to @, so we don't have to
## escape the slashes
cat config | \
    sed 's@^confdir.*@confdir %{_sysconfdir}/%{name}@g' | \
#    sed 's/^permissionsfile.*/permissionsfile \/etc\/%{name}\/permissionsfile/g' | \
#    sed 's/^re_filterfile.*/re_filterfile \/etc\/%{name}\/re_filterfile/g' | \
#    sed 's/^logfile.*/logfile \%{_localstatedir}\/log\/%{name}\/logfile/g' | \
#    sed 's/^jarfile.*/jarfile \%{_localstatedir}\/log\/%{name}\/jarfile/g' | \
#    sed 's/^forward.*/forward \/etc\/%{name}\/forward/g' | \
#    sed 's/^aclfile.*/aclfile \/etc\/%{name}\/aclfile/g' > \
    sed 's@^logdir.*@logdir %{_localstatedir}/log/%{name}@g' > \
    %{buildroot}%{ijbconf}/config
perl -pe 's/{-no-cookies}/{-no-cookies}\n\.redhat.com/' ijb.action >\
    %{buildroot}%{ijbconf}/ijb.action

## Macros are expanded even on commentaries. So, we have to use %%
## -- morcego
#%%makeinstall

%pre
# We check to see if the user junkbuster exists.
# If it does, we do nothing
# If we don't, we check to see if the user junkbust exist and, in case it
# does, we change it do junkbuster. If it also does not exist, we create the
# junkbuster user -- morcego
id junkbuster > /dev/null 2>&1 
if [ $? -eq 1 ]; then
	id junkbust > /dev/null 2>&1 
	if [ $? -eq 0 ]; then
		/usr/sbin/usermod -l junkbuster -d %{_sysconfdir}/%{name} -s "" junkbust  > /dev/null 2>&1
	else
		/usr/sbin/useradd -d %{_sysconfdir}/%{name} -r -s "" junkbuster > /dev/null 2>&1 
	fi
fi

%post
# for upgrade from 2.0.x
[ -f %{_localstatedir}/log/%{name}/%{name} ] &&\
 mv -f %{_localstatedir}/log/%{name}/%{name} %{_localstatedir}/log/%{name}/logfile || /bin/true
chown -R %{name}:%{name} %{_localstatedir}/log/%{name} 2>/dev/null
chown -R %{name}:%{name} /etc/%{name} 2>/dev/null
if [ "$1" = "1" ]; then
     /sbin/chkconfig --add %{name}
	/sbin/service %{name} condrestart > /dev/null 2>&1
fi
# 01/09/02 HB, getting rid of any user=junkbust
# Changed by morcego to use the id command.
id junkbust > /dev/null 2>&1 && /usr/sbin/userdel junkbust || /bin/true

%preun
if [ "$1" = "0" ]; then
	/sbin/service %{name} stop > /dev/null 2>&1 ||:
	/sbin/chkconfig --del %{name}
fi

%postun
#if [ "$1" -ge "1" ]; then
#	/sbin/service %{name} condrestart > /dev/null 2>&1
#fi
# dont forget to remove user and group junkbuster
id junkbuster > /dev/null 2>&1 && /usr/sbin/userdel junkbuster || /bin/true

%clean
[ "%{buildroot}" != "/" ] && rm -rf %{buildroot}

%files
%defattr(0644,root,root,0755)
%doc README AUTHORS ChangeLog
# Where are the webserver/{developer,user}-manual files ?
%doc doc/text/developer-manual.txt doc/text/user-manual.txt
%doc doc/webserver/developer-manual
%doc doc/webserver/user-manual
#%doc %{name}.weekly %{name}.monthly AUTHORS

%dir %{ijbconf}
%dir %{ijbconf}/templates
%attr(0744,junkbuster,junkbuster) %dir %{_localstatedir}/log/%{name}

%attr(0744,junkbuster,junkbuster)%{_sbindir}/%{name}

# We should not use wildchars here. This could mask missing files problems
# -- morcego
%config %{ijbconf}/config
%config %{ijbconf}/ijb-advanced.action
%config %{ijbconf}/ijb-basic.action
%config %{ijbconf}/ijb-intermediate.action
%config %{ijbconf}/ijb.action
%config %{ijbconf}/re_filterfile
%config %{ijbconf}/trust

%config %{ijbconf}/templates/blocked
%config %{ijbconf}/templates/blocked-compact
%config %{ijbconf}/templates/cgi-error-404
%config %{ijbconf}/templates/cgi-error-bad-param
%config %{ijbconf}/templates/cgi-error-disabled
%config %{ijbconf}/templates/cgi-error-file
%config %{ijbconf}/templates/cgi-error-modified
%config %{ijbconf}/templates/cgi-error-parse
%config %{ijbconf}/templates/connect-failed
%config %{ijbconf}/templates/default
%config %{ijbconf}/templates/edit-actions-add-url-form
%config %{ijbconf}/templates/edit-actions-for-url
%config %{ijbconf}/templates/edit-actions-list
%config %{ijbconf}/templates/edit-actions-list-section
%config %{ijbconf}/templates/edit-actions-list-url
%config %{ijbconf}/templates/edit-actions-remove-url-form
%config %{ijbconf}/templates/edit-actions-url-form
%config %{ijbconf}/templates/no-such-domain
%config %{ijbconf}/templates/show-request
%config %{ijbconf}/templates/show-status
%config %{ijbconf}/templates/show-status-file
%config %{ijbconf}/templates/show-url-info
%config %{ijbconf}/templates/show-version
%config %{ijbconf}/templates/toggle
%config %{ijbconf}/templates/toggle-mini
%config %{ijbconf}/templates/untrusted

%config %{_sysconfdir}/logrotate.d/%{name}
%config %attr(0744,root,root) %{_sysconfdir}/rc.d/init.d/%{name}

%{_mandir}/man8/%{name}.8*

%changelog
* Mon Mar 11 2002 Hal Burgiss <hal@foobox.net>
+ junkbuster-2.9.11-8 
- Take out --enable-no-gifs, breaks some browsers.

* Sun Mar 10 2002 Hal Burgiss <hal@foobox.net>
+ junkbuster-2.9.11-8 
- Add --enable-no-gifs to configure.

* Fri Mar 08 2002 Rodrigo Barbosa <rodrigob@tisbrasil.com.br>
+ junkbuster-2.9.11-7
- Added BuildRequires to libtool.

* Tue Mar 06 2002 Rodrigo Barbosa <rodrigob@tisbrasil.com.br>
+ junkbuster-2.9.11-6
- Changed the routined that handle the junkbust and junkbuster users on
  %%pre and %%post to work in a smoother manner
- %%files now uses hardcoded usernames, to avoid problems with package
  name changes in the future

* Tue Mar 05 2002 Rodrigo Barbosa <rodrigob@tisbrasil.com.br>
+ junkbuster-2.9.11-5
- Added "make redhat-dok" to the build process
- Added docbook-utils to BuildRequires

* Tue Mar 05 2002 Rodrigo Barbosa <rodrigob@tisbrasil.com.br>
+ junkbuster-2.9.11-4
- Changing man section in the manpage from 1 to 8
- We now require packages, not files, to avoid issues with apt

* Mon Mar 04 2002 Rodrigo Barbosa <rodrigob@tisbrasil.com.br>
+ junkbuster-2.9.11-3
- Fixing permissions of the init script

* Mon Mar 04 2002 Rodrigo Barbosa <rodrigob@tisbrasil.com.br>
+ junkbuster-2.9.11-2
- General specfile fixup, using the best recomended practices, including:
	- Adding -q to %%setup
	- Using macros whereever possible
	- Not using wildchars on %%files section
	- Doubling the percentage char on changelog and comments, to
	  avoid rpm expanding them

* Sun Mar 03 2002 Hal Burgiss <hal@foobox.net>
- /bin/false for shell causes init script to fail. Reverting.

* Wed Jan 09 2002 Hal Burgiss <hal@foobox.net>
- Removed UID 73. Included user-manual and developer-manual in docs.
  Include other actions files. Default shell is now /bin/false.
  Userdel user=junkbust. ChangeLog was not zipped. Removed 
  RPM_OPT_FLAGS kludge.

* Fri Dec 28 2001 Thomas Steudten <thomas@steudten.ch>
- add paranoia check for 'rm -rf %%{buildroot}'
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

* Mon Sep 25 2000 Stefan Waldherr <stefan@waldherr.org>
- CLF Logging patch by davep@cyw.uklinux.net
- Hal DeVore <haldevore@earthling.net> fix akamaitech in blocklist

* Sun Sep 17 2000 Stefan Waldherr <stefan@waldherr.org>
- Steve Kemp skx@tardis.ed.ac.uk's javascript popup patch.
- Markus Breitenbach breitenb@rbg.informatik.tu-darmstadt.de supplied
  numerous fixes and enhancements for Steve's patch.
- adamlock@netscape.com (Adam Lock) in the windows version:
  - Taskbar activity spinner always spins even when logging is
  turned off (which is the default) - people who don't
  like the spinner can turn it off from a menu option.
  - Taskbar popup menu has a options submenu - people can now
  open the settings files for cookies, blockers etc.
  without opening the JB window.
  - Logging functionality works again
  - Buffer overflow is fixed - new code uses a bigger buffer
  and snprintf so it shouldn't overflow anymore.
- Fixed userid swa, group learning problem while installing.
  root must build RPM.
- Added patch by Benjamin Low <ben@snrc.uow.edu.au> that prevents JB to
  core dump when there is no log file.
- Tweaked SuSE startup with the help of mohataj@gmx.net and Doc.B@gmx.de.
- Fixed man page to include imagefile and popupfile.
- Sanity check for the statistics function added.
- "Patrick D'Cruze" <pdcruze@orac.iinet.net.au>: It seems Microsoft
 are transitioning Hotmail from FreeBSD/Apache to Windows 2000/IIS.
 With IIS/5, it appears to omit the trailing \r\n from http header
 only messages.  eg, when I visit http://www.hotmail.com, IIS/5
 responds with a HTTP 302 redirect header.  However, this header
 message is missing the trailing \r\n.  IIS/5 then closes the
 connection.  Junkbuster, unfortunately, discards the header becomes
 it thinks it is incomplete - and it is.  MS have transmitted an
 incomplete header!
- Added bug reports and patch submission forms in the docs.

* Mon Mar 20 2000 Stefan Waldherr <stefan@waldherr.org>
       Andrew <anw@tirana.freewire.co.uk> extended the JB:
       Display of statistics of the total number of requests and the number
       of requests filtered by junkbuster, also the percentage of requests
       filtered. Suppression of the listing of files on the proxy-args page.
       All stuff optional and configurable.

* Sun Sep 12 1999 Stefan Waldherr <stefan@waldherr.org>
       Jan Willamowius (jan@janhh.shnet.org) fixed a bug in the 
       code which prevented the JB from handling URLs of the form
       user:password@www.foo.com. Fixed.

* Mon Aug  2 1999 Stefan Waldherr <stefan@waldherr.org>
	Blank images are no longer cached, thanks to a hint from Markus 
        Breitenbach <breitenb@rbg.informatik.tu-darmstadt.de>. The user 
        agent is NO longer set by the Junkbuster. Sadly, many sites depend 
        on the correct browser version nowadays. Incorporated many 
	suggestions from Jan "Yenya" Kasprzak <kas@fi.muni.cz> for the
        spec file. Fixed logging problem and since runlevel 2 does not 
        use networking, I replaced /etc/rc.d/rc2.d/S84junkbuster with
        /etc/rc.d/rc2.d/K09junkbuster thanks to Shaw Walker 
        <walker@netgate.net>. You should now be able to build this RPM as 
        a non-root user (mathias@weidner.sem.lipsia.de).

* Sun Jan 31 1999 Stefan Waldherr <stefan@waldherr.org>
	%%{_localstatedir}/log/junkbuster set to nobody. Added /etc/junkbuster/imagelist
	to allow more sophisticated matching of blocked images. Logrotate
	logfile. Added files for auto-updating the blocklist et al.

* Wed Dec 16 1998 Stefan Waldherr <stefan@waldherr.org>
	Configure blank version via config file. No separate blank
	version anymore. Added Roland's <roland@spinnaker.rhein.de>
	patch to show a logo instead of a blank area. Added a suggestion
	from Alex <alex@cocoa.demon.co.uk>: %%{_localstatedir}/lock/subsys/junkbuster.
	More regexps in the blocklist. Prepared the forwardfile for
	squid. Extended image regexp with help from gabriel 
	<somlo@CS.ColoState.EDU>.

* Thu Nov 19 1998 Stefan Waldherr <stefan@waldherr.org>
	All RPMs now identify themselves in the show-proxy-args page.
	Released Windoze version. Run junkbuster as nobody instead of
	root. 

* Fri Oct 30 1998 Stefan Waldherr <stefan@waldherr.org>
	Newest version. First release (hence the little version number
	mixture -- 2.0.2-0 instead of 2.0-7). This version tightens 
	security over 2.0.1; some multi-user sites will need to change 
	the listen-address in the configuration file. The blank version of
        the Internet Junkbuster has a more sophisticated way of replacing
	images. All RPMs identify themselves in the show-proxy-args page.

* Thu Sep 23 1998 Stefan Waldherr <stefan@waldherr.org>
	Modified the blocking feature, so that only GIFs and JPEGs are
	blocked and replaced but not HTML pages. Thanks to 
	"Gerd Flender" <plgerd@informatik.uni-siegen.de> for this nice
	idea. Added numerous stuff to the blocklist. Keep patches in
        seperate files and no longer in diffs (easier to maintain).

* Tue Jun 16 1998 Stefan Waldherr <swa@cs.cmu.edu>
        Moved config files to /etc/junkbuster directory, moved man page,
	added BuildRoot directive (Thanks to Alexey Nogin <ayn2@cornell.edu>)
        Made new version junkbuster-raw (which is only a stripped version of 
        the junkuster rpm, i.e. without my blocklist, etc.)

* Tue Jun 16 1998 (2.0-1)
	Uhm, not that much. Just a new junkbuster version that
	fixes a couple of bugs ... and of course a bigger 
	blocklist with the unique Now-less-ads-than-ever(SM)
	feature.
	Oh, one thing: I changed the default user agent to Linux -- no 
	need anymore to support Apple.

* Tue Jun 16 1998 (2.0-0)
	Now-less-ads-than-ever (SM)
	compiled with gcc instead of cc
	compiled with -O3, thus it should be a little faster
	show-proxy-args now works
	/etc/junkbuster.init wasn't necessary

* Tue Jun 16 1998 (1.4)
	some more config files were put into /etc
	The junkbuster-blank rpm returns a 1x1 pixel image, that gets 
	displayed by Netscape instead of the blocked image.
	Read http://www.waldherr.org/junkbuster/ for
	further info.

* Tue Jun 16 1998 (1.3)
	The program has been moved to /usr/sbin (from /usr/local/bin)
	Init- and stopscripts (/etc/rc.d/rc*) have been added so
	that the junkbuster starts automatically during bootup.
	The /etc/blocklist file is much more sophisticated. Theoretically
	one should e.g. browse all major US and German newspapers without
	seeing one annoying ad.
	junkbuster.init was modified. It now starts junkbuster with an
	additional "-r @" flag.

# $Log: junkbuster-rh.spec,v $
# Revision 1.39  2002/03/08 18:57:29  swa
# remove user junkbuster after de-installation.
#
# Revision 1.38  2002/03/08 13:45:27  morcego
# Adding libtool to Buildrequires
#
# Revision 1.37  2002/03/07 19:23:49  swa
# i hate to scroll. suse: wrong configdir.
#
# Revision 1.36  2002/03/07 05:06:54  morcego
# Fixed %pre scriptlet. And, as a bonus, you can even understand it now. :-)
#
# Revision 1.34  2002/03/07 00:11:57  morcego
# Few changes on the %pre and %post sections of the rh specfile to handle
# usernames more cleanly
#
# Revision 1.33  2002/03/05 13:13:57  morcego
# - Added "make redhat-dok" to the build phase
# - Added docbook-utils to BuildRequires
#
# Revision 1.32  2002/03/05 12:34:24  morcego
# - Changing section internaly on the manpage from 1 to 8
# - We now require packages, not files, to avoid issues with apt
#
# Revision 1.31  2002/03/04 18:06:09  morcego
# SPECFILE: fixing permissing of the init script (broken by the last change)
#
# Revision 1.30  2002/03/04 16:18:03  morcego
# General cleanup of the rh specfile.
#
# %changelog
# * Mon Mar 04 2002 Rodrigo Barbosa <rodrigob@tisbrasil.com.br>
# + junkbuster-2.9.11-2
# - General specfile fixup, using the best recomended practices, including:
#         - Adding -q to %%setup
#         - Using macros whereever possible
#         - Not using wildchars on %%files section
#         - Doubling the percentage char on changelog and comments, to
#           avoid rpm expanding them
#
# Revision 1.29  2002/03/03 19:21:22  hal9
# Init script fails if shell is /bin/false.
#
# Revision 1.28  2002/01/09 18:34:03  hal9
# nit.
#
# Revision 1.27  2002/01/09 18:32:02  hal9
# Removed RPM_OPT_FLAGS kludge.
#
# Revision 1.26  2002/01/09 18:21:10  hal9
# A few minor updates.
#
# Revision 1.25  2001/12/28 01:45:36  steudten
# Add paranoia check and BuildReq: gzip
#
# Revision 1.24  2001/12/01 21:43:14  hal9
# Allowed for new ijb.action file.
#
# Revision 1.23  2001/11/06 12:09:03  steudten
# Compress doc files. Install README and AUTHORS at last as document.
#
# Revision 1.22  2001/11/05 21:37:34  steudten
# Fix to include the actual version for name.
# Let the 'real' packager be included - sorry stefan.
#
# Revision 1.21  2001/10/31 19:27:27  swa
# consistent description. new name for suse since
# we had troubles with rpms of identical names
# on the webserver.
#
# Revision 1.20  2001/10/24 15:45:49  hal9
# To keep Thomas happy (aka correcting my  mistakes)
#
# Revision 1.19  2001/10/15 03:23:59  hal9
# Nits.
#
# Revision 1.17  2001/10/10 18:59:28  hal9
# Minor change for init script.
#
# Revision 1.16  2001/09/24 20:56:23  hal9
# Minor changes.
#
# Revision 1.13  2001/09/10 17:44:43  swa
# integrate three pieces of documentation. needs work.
# will not build cleanly under redhat.
#
# Revision 1.12  2001/09/10 16:25:04  swa
# copy all templates. version updated.
#
# Revision 1.11  2001/07/03 11:00:25  sarantis
# replaced permissionsfile with actionsfile
#
# Revision 1.10  2001/07/03 09:34:44  sarantis
# bumped up version number.
#
# Revision 1.9  2001/06/12 18:15:29  swa
# the %% in front of configure (see tag below) confused
# the rpm build process on 7.1.
#
# Revision 1.8  2001/06/12 17:15:56  swa
# fixes, because a clean build on rh6.1 was impossible.
# GZIP confuses make, %% configure confuses rpm, etc.
#
# Revision 1.7  2001/06/11 12:17:26  sarantis
# fix typo in %%post
#
# Revision 1.6  2001/06/11 11:28:25  sarantis
# Further optimizations and adaptations in the spec file.
#
# Revision 1.5  2001/06/09 09:14:11  swa
# shamelessly adapted RPM stuff from the newest rpm that
# RedHat provided for the JB.
#
# Revision 1.4  2001/06/08 20:54:18  swa
# type with status file. remove forward et. al from file list.
#
# Revision 1.3  2001/06/07 17:28:10  swa
# cosmetics
#
# Revision 1.2  2001/06/04 18:31:58  swa
# files are now prefixed with either `confdir' or `logdir'.
# `make redhat-dist' replaces both entries confdir and logdir
# with redhat values
#
# Revision 1.1  2001/06/04 10:44:57  swa
# `make redhatr-dist' now works. Except for the paths
# in the config file.
#
#
#
