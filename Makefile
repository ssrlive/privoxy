# $Id: Makefile,v 1.1 2001/12/01 11:24:29 jongfoster Exp $
#
# Written by and Copyright (C) 2001 the SourceForge
# Privoxy team.  http://ijbswa.sourceforge.net
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
# $Log: Makefile,v $
# Revision 1.1  2001/12/01 11:24:29  jongfoster
# Will display a warning if non-GNU make is used
#
#

#############################################################################

error: GNUmakefile
	@echo
	@echo "ERROR!"
	@echo "To build this program, you must run ./configure and then run GNU make."
	@echo
	@echo "You are not using the GNU version of Make - maybe it's called gmake"
	@echo "or it's in a different directory?"
	@echo

GNUmakefile:
	@echo
	@echo "ERROR!"
	@echo "To build this program, you must run ./configure and then run GNU make."
	@echo
	@echo "You haven't run ./configure yet."
	@echo
	@false

.PHONY: error

#############################################################################

## Local Variables:
## tab-width: 3
## end:
