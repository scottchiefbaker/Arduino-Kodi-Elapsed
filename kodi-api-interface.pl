#!/usr/bin/env perl

use strict;
use warnings;
use Fcntl qw(O_RDWR);
use POSIX;
use HTTP::Tiny;
use Time::HiRes qw(sleep);
use JSON::PP;

###############################################################################
###############################################################################

my $speed   = 57600;
my $dev     = "/dev/ttyUSB0";
my $kodi_ip = "192.168.5.21";
my $debug   = argv("debug") || 0;

my $player_id = 1; # 0 = Music, 1 = Video, 2 = Picture
my $fh;

if (!$debug) {
	print "Opening $dev @ $speed\n";
	$fh = open_serial_port($dev,$speed);
}

while (1) {
	my $x = get_elapsed();

	if (!$x->{elapsed}) {
		sleep(2);
		next;
	}

	# Format : Elaspsed:Total:PlayMode
	# Example: 1278:3205:Play
	my $cmd = sprintf("%d:%d:%s\n",$x->{elapsed},$x->{total},$x->{playmode});
	print "$cmd";

	if (!$debug) {
		$fh->print($cmd);
	}

	sleep(0.5);
}

###############################################################################
###############################################################################

sub get_elapsed {
	my $url = 'http://' . $kodi_ip . '/jsonrpc?request={"jsonrpc":"2.0","method":"Player.GetProperties","params":{"playerid":' . $player_id . ',"properties":["time","totaltime","percentage","speed"]},"id":"1"}';

	my $resp = HTTP::Tiny->new->get($url);
	my $json = $resp->{content};
	my $x    = decode_json($json);
	my $ec   = $x->{error}->{code} || 0;

	if ($ec == -32100) {
		my $old = $player_id;
		$player_id = get_active_player();

		print "Switching player IDs from $old to $player_id\n";

		return {};
	}

	my $res     = $x->{result};
	my $elapsed = ($res->{time}->{hours} * 3600)      + ($res->{time}->{minutes} * 60)      + $res->{time}->{seconds};
	my $total   = ($res->{totaltime}->{hours} * 3600) + ($res->{totaltime}->{minutes} * 60) + $res->{totaltime}->{seconds};
	my $speed   = $res->{speed};

	my $ret = {};
	$ret->{elapsed} = $elapsed;
	$ret->{total}   = $total;
	$ret->{speed}   = $speed;

	if ($speed == 0 && $total == 0) {
		$ret->{playmode} = "Stop";
	} elsif ($speed == 1) {
		$ret->{playmode} = "Play";
	} elsif ($speed == 0) {
		$ret->{playmode} = "Pause";
	} else {
		$ret->{playmode} = "???";
	}

	return $ret;
}


sub get_active_player {
    my $url = 'http://' . $kodi_ip . '/jsonrpc?request={"jsonrpc":"2.0","id":1,"method":"Player.GetActivePlayers"}';

	my $resp = HTTP::Tiny->new->get($url);
	my $json = $resp->{content};
	my $x    = decode_json($json);

    my $active_id = $x->{result}->[0]->{playerid};

    return $active_id;
}

sub open_serial_port {
	my $dev   = shift();
	my $speed = shift();

	sysopen(my $fh, $dev, O_RDWR) or die("Unable to open $dev\n");

	# Figure out the integer that maps to a given speed
	# perl -E 'use IO::Tty qw( B115200 ); say B115200'
	my $speed_map = {
		9600   => 13,
		19200  => 14,
		38400  => 15,
		57600  => 4097,
		115200 => 4098,
	};

	my $speed_int = $speed_map->{$speed};
	if (!$speed_int) {
		die("Unable to find speed $speed\n");
	}

	# Set the baud on the FH
	my $attr = POSIX::Termios->new;
	$attr->getattr($fh->fileno);
	$attr->setispeed($speed_int);
	$attr->setattr($fh->fileno);

	$fh->autoflush;

	return $fh;
}

###############################################################################
###############################################################################

sub argv {
	my $ret = {};

	for (my $i = 0; $i < scalar(@ARGV); $i++) {
		# If the item starts with "-" it's a key
		if ((my ($key) = $ARGV[$i] =~ /^--?([a-zA-Z_]\w*)/) && ($ARGV[$i] !~ /^-\w\w/)) {
			# If the next item does not start with "--" it's the value for this item
			if (defined($ARGV[$i + 1]) && ($ARGV[$i + 1] !~ /^--?\D/)) {
				$ret->{$key} = $ARGV[$i + 1];
			# Bareword like --verbose with no options
			} else {
				$ret->{$key}++;
			}
		}
	}

	# We're looking for a certain item
	if ($_[0]) { return $ret->{$_[0]}; }

	return $ret;
}

sub trim {
	if (wantarray) {
		my @ret;
		foreach (@_) {
			push(@ret,scalar(trim($_)));
		}

		return @ret;
	} else {
		my $s = shift();
		if (!defined($s) || length($s) == 0) { return ""; }
		$s =~ s/^\s*//;
		$s =~ s/\s*$//;

		return $s;
	}
}

# Debug print variable using either Data::Dump::Color (preferred) or Data::Dumper
# Creates methods k() and kd() to print, and print & die respectively
BEGIN {
	if (eval { require Data::Dump::Color }) {
		*k = sub { Data::Dump::Color::dd(@_) };
	} else {
		require Data::Dumper;
		*k = sub { print Data::Dumper::Dumper(\@_) };
	}

	sub kd {
		k(@_);

		printf("Died at %2\$s line #%3\$s\n",caller());
		exit(15);
	}
}

# String format: '115', '165_bold', '10_on_140', 'reset', 'on_173'
sub color {
	my $str = shift();

	# If we're NOT connected to a an interactive terminal don't do color
	if (-t STDOUT == 0) { return ''; }

	# No string sent in, so we just reset
	if (!length($str) || $str eq 'reset') { return "\e[0m"; }

	# Some predefined colors
	my %color_map = qw(red 160 blue 21 green 34 yellow 226 orange 214 purple 93 white 15 black 0);
	$str =~ s|([A-Za-z]+)|$color_map{$1} // $1|eg;

	# Get foreground/background and any commands
	my ($fc,$cmd) = $str =~ /(\d+)?_?(\w+)?/g;
	my ($bc)      = $str =~ /on_?(\d+)/g;

	# Some predefined commands
	my %cmd_map = qw(bold 1 italic 3 underline 4 blink 5 inverse 7);
	my $cmd_num = $cmd_map{$cmd // 0};

	my $ret = '';
	if ($cmd_num)     { $ret .= "\e[${cmd_num}m"; }
	if (defined($fc)) { $ret .= "\e[38;5;${fc}m"; }
	if (defined($bc)) { $ret .= "\e[48;5;${bc}m"; }

	return $ret;
}

# vim: tabstop=4 shiftwidth=4 autoindent softtabstop=4
