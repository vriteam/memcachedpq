<?php
$pid = pcntl_fork();
if( $pid )
	exit;
$m = memcache_connect('10.8.8.41');
$key_prefix = 'priority_';
do{
	$rand = mt_rand( 1, 100);
	if( $rand % 2 === 0)
		memcache_add($m, $key_prefix . $rand, $rand, $rand);
	else{
		memcache_add($m, $key_prefix . $rand, $rand, $rand);
		memcache_get($m, 'maxpriority');
	}
}while(true);

