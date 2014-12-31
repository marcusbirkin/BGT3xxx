#!/usr/bin/perl

$ADAPTER = 0;

&dvbnet($ADAPTER, 0,  512, "192.168.11.1");
&dvbnet($ADAPTER, 0, 2000, "192.168.21.1");


# &dvbnet(adapter,netdev,pid,"ip_addr");

sub dvbnet
{
  local ($ADAPTER, $NETDEV, $PID, $IP_ADDR) = @_;

  $DEV_NAME = `./dvbnet -a $ADAPTER -n $NETDEV -p $PID | grep created`;
  chop($DEV_NAME);

  $DEV_NAME =~ s/(.*)device //;
  $DEV_NAME =~ s/for (.*)//;

  $X = `/sbin/ifconfig $DEV_NAME $IP_ADDR netmask 255.255.255.0`;

  system("/sbin/ifconfig $DEV_NAME");
}
