#!/usr/bin/perl

#
# ...because it's easier to write my own internationalization
# support than understanding gnu gettext and all the incompatibilities
# between its versions
#

my $msgcount = 0;

my $id  = "";
my $msg = "";

my $state = 0;

while($_=<>) {
    chomp;

    if ($state == 0) {
	if (/^msgid\s+\"(.*)\"/) {
	    $id  = $1;
	    $msg = "";
	    $state = 1;
	    next;
	}
	next;
    }

    if ($state == 1) {
	if (/^\"(.*)\"/) {
	    $id = "$id$1";
	    next;
	}
	if (/^msgstr\s+\"(.*)\"/) {
	    $msg = $1;
	    $state = 2;
	    next;
	}
	next;
    }

    if ($state == 2) {
	if (/^\"(.*)\"/) {
	    $msg = "$msg$1";
	    next;
	}

	$id  =~ s/\\"/"/g;
	$msg =~ s/\\"/"/g;
	
	$idlen = length($id);
	$msglen = length($msg);
	if ($idlen > 0 && $msglen > 0) {
	    $msgcount++;
	    print "L$idlen $msglen\n";
	    print "$id\n";
	    print "$msg\n";
	}
	$state = 0;
	next;
    }
}

if ($state != 0) {
    $idlen = length($id);
    $msglen = length($msg);
    if ($idlen > 0 && $msglen > 0) {   
	$msgcount++;
	print "L$idlen $msglen\n";
	print "$id\n";
	print "$msg\n";
    }
}

print "E $msgcount\n";


