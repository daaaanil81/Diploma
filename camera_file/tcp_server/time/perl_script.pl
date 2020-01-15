#!/usr/bin/perl

use strict;
use warnings;
use Socket;
use Digest::SHA qw(hmac_sha1);
use Digest::CRC qw(crc32);
#use Socket6;

my ($ip_server, $port_server, $ip_browser, $port_browser, $pwd, $username) = @ARGV;
print("Result test: IP = $ip_browser:$port_browser\nPWD: $pwd\n");


my @rand = ('A' .. 'Z', 'a' .. 'z');
my $ufrag = "sEMT"; 
my $tract = join('', (map {$rand[rand($#rand)]} (1 .. 12)));
my $tbreak = int(rand(0xffffffff)) * int(rand(0xffffffff));
my $control = 1;
my $prio = 1853817087;

print("transaction: $tract\n");
print("my username fragment: $ufrag\n");
print("controll".($control?"ing":'ed')."\n");
print("tie breaker: $tbreak\n");
print("Username: $username\n");


my $packet = '';
$packet .= attr(0x0006, "$username:$ufrag");
$packet .= attr($control ? 0x802a : 0x8029, pack('Q', $tbreak)); # ice-controlling / ice-controlled
$packet .= use_i(); 
$packet .= attr(0x24, pack('N', $prio));
$packet .= integrity();
$packet .= fingerprint();
$packet = header() . $packet;

my $fd;
if (!socket($fd, PF_INET, SOCK_DGRAM, getprotobyname('udp'))) {
	undef($fd);
	print("Error in Socket\n");
	exit 1;
}
my $iaddr_browser = inet_aton($ip_browser);
my $paddr_browser = sockaddr_in($port_browser, $iaddr_browser);
my $iaddr_server = inet_aton($ip_server);
my $paddr_server = sockaddr_in($port_server, $iaddr_server);

bind($fd, $paddr_server);

if (!connect($fd, $paddr_browser)) {
	undef($fd);
	print("Error in Connect\n");
	exit 1;
}
send($fd, $packet, 0); 

exit 0;

sub use_i {
	my $type = 0x0025;
	my $len = 0;
	return pack('nn', $type, $len);
}
	
sub attr {
	my ($type, $data) = @_;
	my $len = length($data);
	while ((length($data) % 4) != 0) {
		$data .= "\0";
	}
	return pack('nn a*', $type, $len, $data);
}
sub header {
	my ($add_length) = @_;
	$add_length ||= 0;
	return pack('nnN a12', 1, length($packet) + $add_length, 0x2112A442, $tract);
}
sub integrity {
	my $h = header(24);
	my $hmac = hmac_sha1($h.$packet, $pwd);
	return attr(8, $hmac);
}
sub fingerprint {
	my $h = header(8);
	my $crc = crc32($h.$packet);
	return attr(0x8028, pack('N', ($crc ^ 0x5354554e)));
}
