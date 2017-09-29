#!/usr/bin/perl

## harvest.pl: retrieve translatable messages from the input
##             files listed in harvest.in
##             and rewrite all .po files updating the translation
##             status

sub add_message;
sub unescape_message;
sub wrap_message;
sub extract_messages;
sub update_pos;
sub update_po;

################
#
# globals
#
################

my $package  = '';
my $outfile  = '';
my $langs    = '';
my @srcfiles = ( );
my @messages  = ( );
my %locations = ( );
my %rcount    = ( );
my @langs    = ( );

extract_messages;
update_pos;

##############
#
# SUBS
#
##############

sub update_pos {
    my $x;
    @langs = split(/,/,$langs);
    
    print "Updating po files...\n";
    for (@langs) {
	$x = $package . '.' . $_ . ".po";
	print "  $x\n";
	update_po($x);
    }
    return 0;
}

sub update_po {
    my $po = shift @_;
    my @pomsg = ( );
    my %potrans = ( );
    my %status = ( );
    my %comment = ( );
    my $id, $msg, $state, $idlen, $msglen, $lastcomment, $M, $N;
    my $t, $u, $l;
    my @W;

    if (!open(PO,"<$po")) {
	print "** unable to open $po.\n";
	exit 2;
    }

    $state = 0;
    $id  = '';
    $msg = '';
    $lastcomment = '';
    while($_=<PO>) {
	chomp;

	if (/^\#/) {
	    $lastcomment = $_ unless (/^\#,/);
	    next;
	}
	
	if ($state == 0) {
	    if (/^msgid\s+\"(.*)\"/) {
		$id  = $1;
		$msg = '';
		$state = 1;
		next;
	    }
	    next;
	}

	if ($state == 1) {
	    if (/^\"(.*)\"/) {
		$id = $id . $1;
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
		$msg = $msg . $1;
		next;
	    }
	    
	    $idlen  = length($id);
	    $msglen = length($msg);
	    if ($idlen > 0 && $msglen > 0) {
		push @pomsg, $id;
		$potrans{$id} = $msg;
		$comment{$id} = $lastcomment;
	    }
	    $state = 0;
	    next;
	}
    }

    if ($state != 0) {
	$idlen  = length($id);
	$msglen = length($msg);
	if ($idlen > 0 && $msglen > 0) {
	    push @pomsg, $id;
	    $potrans{$id} = $msg;
	    $comment{$id} = $lastcomment;
	}
    }

    close(PO);

    # now find state of each
    for $M (@messages) {
	$status{$M} = 'UNTRANSLATED';
    }
    for $M (@pomsg) {
	$status{$M} = 'LOST';
    }
    
    for $M (@pomsg) {
	for $N (@messages) {
	    if ($M eq $N) {
		$status{$M} = 'TRANSLATED';
		last;
	    }
	}
    }

    for $M (@messages) {
	if ($status{$M} eq 'UNTRANSLATED') {
	    push @pomsg, $M;
	    $comment{$M} = "# ($rcount{$M}) $locations{$M}";
	    next;
	}
	if ($status{$M} eq 'TRANSLATED') {
	    $comment{$M} = "# ($rcount{$M}) $locations{$M}";
	}
    }
    
    @pomsg = sort @pomsg;

    $t = 0;
    $u = 0;
    $l = 0;

    if (!open(PO,">$po")) {
	print "** unable to write to $po.\n";
	exit 2;
    }

    for $M (@pomsg) {

	if ($status{$M} eq 'TRANSLATED') {
	    $t++;
	    print PO "# state: translated\n";
	    print PO "$comment{$M}\n" if ($comment{$M});
	    if (length($M) < 74) {
		print PO "msgid   \"$M\"\n";
		print PO "msgstr  \"$potrans{$M}\"\n\n";
	    } else {
		@W = wrap_message($M);
		print PO "msgid  \"\"\n";
		for (@W) {
		    print PO "\"$_\"\n";
		}
		@W = wrap_message($potrans{$M});
		print PO "msgstr \"\"\n";
		for (@W) {
		    print PO "\"$_\"\n";
		}
		print PO "\n";
	    }
	    next;
	}

	if ($status{$M} eq 'LOST') {
	    $l++;
	    print PO "# state: lost (deprecated key)\n";
	    print PO "$comment{$M}\n" if ($comment{$M});
	    if (length($M) < 74) {
		print PO "msgid   \"$M\"\n";
		print PO "msgstr  \"$potrans{$M}\"\n\n";
	    } else {
		@W = wrap_message($M);
		print PO "msgid  \"\"\n";
		for (@W) {
		    print PO "\"$_\"\n";
		}
		@W = wrap_message($potrans{$M});
		print PO "msgstr \"\"\n";
		for (@W) {
		    print PO "\"$_\"\n";
		}
		print PO "\n";
	    }
	    next;
	}

	if ($status{$M} eq 'UNTRANSLATED') {
	    $u++;
	    print PO "# state: untranslated\n";
	    print PO "$comment{$M}\n" if ($comment{$M});
	    if (length($M) < 74) {
		print PO "msgid   \"$M\"\n";
		print PO "msgstr  \"\"\n\n";
	    } else {
		@W = wrap_message($M);
		print PO "msgid  \"\"\n";
		for (@W) {
		    print PO "\"$_\"\n";
		}
		print PO "msgstr \"\"\n\n";
	    }
	    next;
	}
    } # for M in pomsg

    close(PO);
    print "done, $t translated, $u untranslated, $l lost/deprecated.\n";
} # sub

sub extract_messages {
    my $state    = 0;
    my $line     = 0;
    my $xline    = 0;
    my $msgbuf   = '';
    my $warnings = 0;

    if (!open(HIN,"<harvest.in")) {
	print "** unable to open harvest.in\n";
	exit 2;
    }
    
    while($_=<HIN>) {
	chomp;
	next if (/^\#/);
	if (/^package=(\S+)/) {
	    $package = $1;
	    next;
	}
	if (/^langs=(\S+)/) {
	    $langs = $1;
	    next;
	}
	if (/\s*(\S+)\s*/) {
	    push @srcfiles, $_;
	}
    }
    
    close(HIN);
    
    if ($package eq '') {
	print "** error: harvest.in did not specify an output file.\n";
	exit 2;
    }
    
    $outfile = $package . ".hemp";
    
    for my $src (@srcfiles) {
	print "reading $src...\n";
	if (!open(SRC,"<$src")) {
	    print "** unable to open file.\n";
	    exit 2;
	}
	$state  = 0;
	$line   = 0;
	$msgbuf = '';
	
	while($_=<SRC>) {
	    chomp;
	    ++$line;
	    if (/_\(.*\).*_\(/) {
		print "** WARNING: I'm just a poor perl script, not a C compiler! Please don't start two translatables in the same line, $src:$line -- continuing anyway, result may be wrong.\n";
		$warnings++;
	    }
	    
	    if ($state == 0) {
		if (/_\(\s*\"(.*)\"\s*\)/) {
		    $state = 0;
		    $msgbuf = $1;
		    add_message($msgbuf,$src,$line);
		    next;
		}
		if (/_\(\s*\"(.*)\"\\/) {
		    $state  = 1;
		    $msgbuf = $1;
		    $xline  = $line;
		    next;
		}
	    } else {
		if (/^\s*\"(.*)\"\s*\)/) {
		    $state   = 0;
		    $msgbuf .= $1;
		    add_message($msgbuf,$src,$xline);
		    next;
		}
		if (/^\s*\"(.*)\"\\/) {
		    $state   = 1;
		    $msgbuf .= $1;
		    next;
		}
		print "** unparseable multi-line string at $src:$line\n";
		print "** dump [$_]\n";
		exit 2;
	    }
	}
	close(SRC);
    }
    
    print "sorting messages...\n";
    @messages = sort @messages;
    
    print "writing $outfile...\n";
    
    if (!open(HEMP,">$outfile")) {
	print "** unable to open $outfile for writing.\n";
	exit 2;
    }
    
    for my $m (@messages) {
	print HEMP "# ($rcount{$m}) $locations{m}\n";
	
	if (length($m) < 74)  {
	    print HEMP "msgid  \"$m\"\n";
	} else {
	    @M = wrap_message($m);
	    print HEMP "msgid  \"\"\n";
	    for (@M) {
		print HEMP "\"$_\"\n";
	    }	
	}
	
	print HEMP "msgstr \"\"\n\n";
    }
    
    close(HEMP);
    print "extraction done.\n";
    $mcount = 1 + $#messages;
   print "summary: $warnings warnings, $mcount distinct messages.\n";    
}

sub add_message {
    my $msg = shift @_;
    my $loc = shift @_;
    my $lin = shift @_;
    my $emsg, $eloc, $m;

    # remove path components from locations
    $_ = $loc;
    while (/.*\/(.*)/) {
	$_ = $1;
    }
    $loc = $_;

    $emsg = unescape_message($msg);
    $eloc = "$loc:$lin";

    # check for repeated messages
    for $m (@messages) {
	if ($m eq $emsg) {
	    $rcount{$emsg}++;

	    $_ = $locations{$emsg};
	    if (/(\S+):\S+$/) {
		if ($1 eq $loc) {
		    $eloc = ",$lin";
		} else {
		    $eloc = "; $eloc";
		}
	    } else {
		$eloc = "; $eloc-HUH?!?";
	    }
	    $locations{$emsg} .= $eloc;
	    return 0;
	}
    }

    # new message, really add it
    push @messages,  $emsg;
    $locations{$emsg} = $eloc;
    $rcount{$emsg} = 1;
    return 1;
}

# unescape all C backslash escapes, except \n
sub unescape_message {
    my $x = shift @_;
    my $value, $tmp, $code;

    # octal trithings
    $_ = $x;
    while (/\\(\d{3})/) {
	$code = $1;
	$value = 0;
	$tmp = substr($code, 0, 1);
	$value = 64 * $tmp;
	$tmp = substr($code, 1, 1);
	$value += 8 * $tmp;
	$tmp = substr($code, 2, 1);
	$value += $tmp;
	$tmp = chr($value);
	$x =~ s/\\$code/$tmp/g;
	$_ = $x;
    }

    # \a \r \t \0 \b \v \f \\
    # man 7 ascii has a nice list
    # \n is left escaped
    $tmp = chr(0);  $x =~ s/\\0/$tmp/g;
    $tmp = chr(7);  $x =~ s/\\a/$tmp/g;
    $tmp = chr(8);  $x =~ s/\\b/$tmp/g;
    $tmp = chr(9);  $x =~ s/\\t/$tmp/g;
    $tmp = chr(11); $x =~ s/\\v/$tmp/g;
    $tmp = chr(12); $x =~ s/\\f/$tmp/g;
    $tmp = chr(13); $x =~ s/\\r/$tmp/g;
    $tmp = chr(92); $x =~ s/\\\\/$tmp/g;
    
    return($x);
}

sub wrap_message {
    my $x = shift @_;
    my $y, $z;
    my @r = ( );

    $_ = $x;
    if (/^(.{3,}?\\n)(.+)/) {
	$y = $1;
	$z = $2;
	@r = wrap_message($z);
	@r = ( $y, @r );
	return @r;
    } else {
	@r = ( $x );
	return @r;
    }
}
