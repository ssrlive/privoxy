# $Id: junkbuster-suse.spec,v 1.2 2001/06/07 17:18:44 swa Exp $
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
Group:        Networking/Utilities
Provides:     ijb
Obsoletes:    ijb
Autoreqprov:  on
Version: 2.9
Release: 4
Summary:      The Internet Junkbuster
Source:  http://www.waldherr.org/junkbuster/ijbswa.tar.gz
atch:   ijb20.dif

#
# -----------------------------------------------------------------------------
#
%description
The Internet Junkbuster (TM) blocks unwanted banner ads and protects
your privacy from cookies and other threats. It's free under the GPL
(no warranty), runs under *NIX and works with almost any browser. You
need to clear you browser's cache and specify the proxy-server,
described in /usr/doc/junkbuster.  This is a modified version which
returns a blank GIF for blocked images by default.  But you can
configure this via /etc/junkbuster/config.

Authors:
--------
    http://ijbswa.sourceforge.net

SuSE series: n

#
# -----------------------------------------------------------------------------
#
%prep
%setup -n ijb20
%patch

#
# -----------------------------------------------------------------------------
#
%build
make
cat > /etc/init.d/junkbuster << EOT
#! /bin/sh
# Copyright (c) 1999 SuSE GmbH Nuremberg, Germany.  All rights reserved.
#
# Author: Daniel Bischof <daniel@suse.de>, 1999
#
# /sbin/init.d/junkbuster
#
### BEGIN INIT INFO
# Provides:       junkbuster ijb
# Required-Start: $network syslog
# Required-Stop:
# Default-Start:  3 5
# Default-Stop:
# Description:    Starts the Internet Junkbuster
### END INIT INFO
. /etc/rc.config
base=\${0##*/}
link=\${base#*[SK][0-9][0-9]}
#test \$link = \$base && START_IJB=yes
#test "\$START_IJB" = "yes" || exit 0
return=\$rc_done
case "\$1" in
    start)
        echo -n "Starting The Internet Junkbuster"
        su - nobody -c 'nohup /usr/sbin/junkbuster /etc/ijb/junkbstr.ini < /dev/null > /dev/null &'
        sleep 1
        echo -e "\$return"
        ;;
    stop)
        echo -n "Shutting down The Internet Junkbuster"
        killproc -TERM /usr/sbin/junkbuster || return=\$rc_failed
        echo -e "\$return"
        ;;
    restart|reload)
        \$0 stop && \$0 start || return=\$rc_failed
        ;;
    status)
        checkproc /usr/sbin/junkbuster && echo OK || echo No process
        ;;
    *)
        echo "Usage: \$0 {start|restart|status|stop}"
        exit 1
esac
test "\$return" = "\$rc_done" || exit 1
exit 0
EOT
chmod 755 /etc/init.d/junkbuster
ln -sf /etc/init.d/junkbuster /usr/sbin/rcjunkbuster

#
# -----------------------------------------------------------------------------
#
%install
install -m 755 junkbuster /usr/sbin
install -d /etc/ijb
install -m 644 *.ini /etc/ijb
install -m 644 junkbuster.1 /usr/share/man/man1
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
%doc README *.html
/usr/sbin/junkbuster
/usr/share/man/man1/junkbuster.1.gz
%config(noreplace) /etc/ijb
/etc/init.d/junkbuster
/usr/sbin/rcjunkbuster

#
# -----------------------------------------------------------------------------
#
%changelog -n junkbuster
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
