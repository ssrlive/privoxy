#!/bin/sh

VERSION=`cat privoxy-rh.spec | sed -n -e 's/^Version:[ ]*//p'`
RELEASE=`cat privoxy-rh.spec | sed -n -e 's/^Release:[ ]*//p'`
CLTAG=${VERSION}-${RELEASE}cl

PACKAGER=`rpm --eval "%{packager}"`
if [ "${PACKAGER}" = "%{packager}" ]; then
	PACKAGER="genclspec script <developers@privoxy.org>"
fi

export LC_ALL=
export LANG=
DATETAG=`date "+%a %b %d %Y"`

if [ -r privoxy-cl.spec ]; then
	echo Old CL specfile found. Removing it.
fi

cat privoxy-rh.spec | sed -e 's/^\(Release:[ ]*[^ ]\+\)[ ]*$/\1cl/' \
			  -e "/^%changelog/a* ${DATETAG} ${PACKAGER}" \
			  -e "/^%changelog/a+ privoxy-${CLTAG}" \
			  -e "/^%changelog/a- Packaging for Conectiva Linux (automatic genarated specfile)" \
			  -e '/^%changelog/a \
' > privoxy-cl.spec

