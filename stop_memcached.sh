#!/bin/bash
pidfile=/tmp/memcached.pid
if [ -r $pidfile ];then
pid=`/bin/cat $pidfile`
kill $pid
/bin/rm -f $pidfile
else
echo "no pid file"
fi
