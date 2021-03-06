#!/usr/bin/env perl

use strict;
use warnings;
use Fcntl qw(O_RDWR);
use POSIX;
use HTTP::Tiny;
use Time::HiRes qw(sleep);
use JSON::PP;
use POSIX qw(setsid);

###############################################################################
###############################################################################

my $speed     = 57600;
my $dev       = argv("dev") || "/dev/ttyUSB0";
my $kodi_ip   = "192.168.5.21";

################################################

my $debug     = argv("debug")  || 0;
my $start     = argv("start")  || 0;
my $delay     = argv("delay")  || 0.05;
my $daemon    = $start;
my $pid_file  = "/tmp/kodi-api.pid";
my $player_id = 1; # 0 = Music, 1 = Video, 2 = Picture
my $fh;

if (!$debug) {
	print "Connecting to Arduino on $dev @ $speed\n";
	$fh = open_serial_port($dev,$speed);
}

# Test mdoe
if (argv("test")) {
	test_mode("long");
} elsif (argv("short_test")) {
	test_mode("short");
# Start the daemon up
} elsif ($start) {
	start_daemon();
# Stop the daemon
} elsif (argv("stop")) {
	stop_daemon();
}

while (1) {
	my $x = get_elapsed();

	# Error from get_elapsed()
	if (!defined($x->{elapsed})) {
		sleep(2);
		next;
	}

	# Send the command to the serial port
	send_command($x->{elapsed}, $x->{total}, $x->{playmode});

	sleep(0.45);
}

###############################################################################
###############################################################################

sub stop_daemon {
	my $pid = is_running();

	if (!$pid) {
		exit(9);
	}

	print "Stopping daemon: pid $pid\n";
	my $ok = kill('KILL', $pid);

	if ($ok) {
		unlink($pid_file);
	} else {
		die("Unable to stop daemon\n");
	}

	# Clear the display
	send_command(0,0,"Stop");
	sleep(0.4);

	exit();
}

sub start_daemon {
	my $already_running = is_running();
	if ($already_running) {
		exit(5);
	}

	# Simple daemon stuff borrowed from: https://www.linuxquestions.org/questions/programming-9/how-to-run-a-perl-script-as-a-daemon-109978/
	chdir '/';
	umask 0;
	open(STDIN,  "<", "/dev/null");
	open(STDERR, ">", ">/dev/null");
	my $pid = fork;

	if ($pid) {
		print "Starting daemon: pid $pid\n";
		open(my $fh, ">", $pid_file) or die("Cannot write to pid file $pid_file");
		print $fh $pid;
	}

	exit if $pid;
	setsid;
}

sub get_elapsed {
	while ($player_id < 0) {
		print "Error grabbing player_id... pausing for 1 second\n";
		$player_id = get_active_player();

		sleep(1);
	}

	my $url = 'http://' . $kodi_ip . '/jsonrpc?request={"jsonrpc":"2.0","method":"Player.GetProperties","params":{"playerid":' . $player_id . ',"properties":["time","totaltime","percentage","speed"]},"id":"1"}';

	my $resp = HTTP::Tiny->new->get($url);
	my $json = $resp->{content};

	my $x;
	eval { $x = decode_json($json); };

	my $ec  = $x->{error}->{code} || 0;
	my $res = $x->{result}        || "";

	# No result or error code something went wrong
	if (!$res && !$ec) {
		return {};
	}

	# If there is an error, try and see if the player_id has changed (ie. video -> music)
	if ($ec == -32100) {
		sleep(1);

		my $old    = $player_id;
		$player_id = get_active_player();

		print "Switching player IDs from $old to $player_id\n";

		return {};
	}

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

# Check if the daemon is already running
# Return 0 if not, and the pid number if it is
sub is_running {
	if (!-r($pid_file)) {
		return 0;
	}

	open(my $fh, "<", $pid_file);
	my $line = <$fh>;
	my $pid  = trim($line);

	# Check it the pid is active
	my $running = kill(0, $pid);

	my $ret;
	if ($running) {
		$ret = $pid;
	} else {
		$ret = 0;
	}

	return $ret;
}

# Do a test of the display (real fast count up)
sub test_mode {
	my $mode = shift();

	if ($mode eq "short") {
		for (my $i = 0 ; $i <= 10 ; $i++) {
			send_command($i,10,"Play");
			sleep(1);
		}

		while (1) {
			send_command(0,0,"Stop");
			sleep(1);
		}
	} elsif ($mode eq "long") {
		my $max = 6100;
		for (my $i = 1 ; $i <= $max; $i++ ) {
			send_command($i,$max,"Play");

			sleep($delay);
		}

		for (my $i = 1 ; $i <= 140; $i++ ) {
			send_command(0,0,"Stop");
			sleep($delay);
		}
	}

	exit(0);
}

# Send a command to the Arduino via the serial port
sub send_command {
	my $elapsed   = shift();
	my $total     = shift();
	my $play_mode = shift();

	# Format : Elaspsed:Total:PlayMode
	# Example: 1278:3205:Play
	my $cmd = sprintf("<%d:%d:%s>\n", $elapsed, $total, $play_mode);

	# Print out the stats
	if (!$daemon) {
		print $cmd;
	}

	if (!$debug) {
		$fh->print($cmd);
	}
}

# Figure out which player Kodi is using (0 = Audio, 1 = Video, 2 = Picture)
sub get_active_player {
    my $url = 'http://' . $kodi_ip . '/jsonrpc?request={"jsonrpc":"2.0","id":1,"method":"Player.GetActivePlayers"}';

	my $resp = HTTP::Tiny->new->get($url);
	my $json = $resp->{content};

	my $x;
	eval { $x = decode_json($json); };

    my $active_id = $x->{result}->[0]->{playerid} // -1;

    return $active_id;
}

# Open a serial port at a given speed and return a filehandle
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

# vim: tabstop=4 shiftwidth=4 autoindent softtabstop=4
