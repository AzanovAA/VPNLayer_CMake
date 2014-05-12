#!/bin/sh

scutil_query()
{
    key=$1

    scutil<<EOT
    open
    get $key
    d.show
    close
EOT
}

SERVICE_GUID=`scutil_query State:/Network/Global/IPv4 | awk '/PrimaryService/ {print $3}'`
SERVICE_ROUTER=`scutil_query State:/Network/Global/IPv4 | awk -F': ' '/Router/ {print $2}'`
SERVICE_NAME=`scutil_query Setup:/Network/Service/$SERVICE_GUID | awk -F': ' '/UserDefinedName/ {print $2}'`

if [ "$1" == 'gateway' ] ; then
	echo "$SERVICE_ROUTER";
fi

if [ "$1" == 'service' ] ; then
	echo "$SERVICE_NAME";
fi

if [ "$1" == 'update' ] ; then # XXX insecure and can't be fixed :(
	OLDDIR="$(dirname $(dirname $(dirname $(pwd))))" 
	TMPDIR="/tmp/.85C21676-1B67-4A2A-A01C-D874E5B3EA4B"

	rm -rf "$TMPDIR"
	mkdir "$TMPDIR"
	mv "/tmp/$2" "$TMPDIR"
	cd "$TMPDIR"
	unzip "$2"
	rm "$2"
	cd *
	NEWDIR=$(pwd)
	cd /tmp

	[ -d "$NEWDIR/Contents" ]                                     \
	 && mv "$OLDDIR" "/tmp/.12C64735-987C-4157-A42F-271E3EBB0BC4" \
	 && mv "$NEWDIR" "$OLDDIR"                                    \
	 && rm -rf "/tmp/.12C64735-987C-4157-A42F-271E3EBB0BC4"       \
	 && rm -rf "$TMPDIR"                                          \
	 && cd "$OLDDIR/Contents/Resources/Bin"                       \
	 && "./settings" fix_permissions
fi

if [ "$1" == 'fix_permissions' ] ; then
	chmod -R ugo+rX     ../../..
	chown -R root:wheel ../Kext/*
	chown -R root:wheel ../Bin/*
	chmod 4755          ../Bin/fping
	chmod 4755          ../Bin/start
	chmod 4755          ../Bin/stop
	chmod 4755          ../Bin/settings
fi

if [ "$1" == 'unload_kexts' ] ; then
	# try to remove tun/tap kexts from other apps
	kextstat -lk | awk '/[^A-Za-z](tun|tap)[^A-Za-z]/ {print $6}' | xargs -n1 kextunload -b
fi

if [ "$1" == 'routes_on' ] ; then
	./route.pl add $2
fi
if [ "$1" == 'routes_off' ] ; then
	./route.pl delete $2
fi

if [ "$1" == 'dns_init' ] ; then
	mkdir -p /var/run/vpnlayer
	networksetup -getdnsservers "$SERVICE_NAME" > /var/run/vpnlayer/dns
fi
if [ "$1" == 'dns_on' ] ; then
	networksetup -setdnsservers "$SERVICE_NAME" $2 $3 $4
fi
if [ "$1" == 'dns_off' ] ; then
	networksetup -setdnsservers "$SERVICE_NAME" empty
	networksetup -setdnsservers "$SERVICE_NAME" `cat /var/run/vpnlayer/dns`
fi

if [ "$1" == 'proxy_on' ] ; then
	networksetup -setsocksfirewallproxy "$SERVICE_NAME" "$2" "$3"
	networksetup -setsocksfirewallproxystate "$SERVICE_NAME" on
fi
if [ "$1" == 'proxy_off' ] ; then
	networksetup -setsocksfirewallproxystate "$SERVICE_NAME" off
fi
