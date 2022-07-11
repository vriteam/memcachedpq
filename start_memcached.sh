#!/bin/bash
for i in /usr/local/mpq /usr /usr/local/memcached;do
if [ -x "$i/bin/memcached" ];then
break
fi
done
$i/bin/memcached -m 200 -Q 1024 -u daemon -d -P /tmp/memcached.pid -p 42001
