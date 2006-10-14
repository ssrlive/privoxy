#! /bin/sh

### BEGIN INIT INFO
# Provides:          privoxy
# Required-Start:    $local_fs $remote_fs $network $time
# Required-Stop:     $local_fs $remote_fs $network $time
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Privacy enhancing HTTP Proxy
# Description:       Privoxy is a web proxy with advanced filtering
#                    capabilities for protecting privacy, filtering
#                    web page content, managing cookies, controlling
#                    access, and removing ads, banners, pop-ups and
#                    other obnoxious Internet junk.
### END INIT INFO

PATH=/sbin:/bin:/usr/sbin:/usr/bin
DAEMON=/usr/sbin/privoxy
NAME=privoxy
DESC="filtering proxy server"
OWNER=privoxy
CONFIGFILE=/etc/privoxy/config
PIDFILE=/var/run/$NAME.pid

test -f $DAEMON || exit 0

set -e

case "$1" in
  start)
	echo -n "Starting $DESC: "
	start-stop-daemon --oknodo --start --quiet --pidfile $PIDFILE \
	    --exec $DAEMON -- --pidfile $PIDFILE --user $OWNER $CONFIGFILE \
	    2>> /var/log/privoxy/errorfile
 	echo "$NAME."
	;;

  stop)
	echo -n "Stopping $DESC: "
	start-stop-daemon --oknodo --stop --quiet --pidfile $PIDFILE \
		--exec $DAEMON
	rm -f $PIDFILE
	echo "$NAME."
	;;

  restart|force-reload)
	echo -n "Restarting $DESC: "
	start-stop-daemon --oknodo --stop --quiet --pidfile $PIDFILE \
		--exec $DAEMON
	sleep 1
	start-stop-daemon --oknodo --start --quiet --pidfile $PIDFILE \
	    --exec $DAEMON -- --pidfile $PIDFILE --user $OWNER $CONFIGFILE \
	    2>> /var/log/privoxy/errorfile
	echo "$NAME."
	;;

  status)
        echo -n "Status of $DESC: "
        if [ ! -r "$PIDFILE" ]; then
                echo "$NAME is not running."
                exit 3
        fi
        if read pid < "$PIDFILE" && ps -p "$pid" > /dev/null 2>&1; then
                echo "$NAME is running."
                exit 0
        else
                echo "$NAME is not running but $PIDFILE exists."
                exit 1
        fi
        ;;

  *)
	N=/etc/init.d/$NAME
	echo "Usage: $N {start|stop|restart|force-reload|status}" >&2
	exit 1
	;;
esac

exit 0
