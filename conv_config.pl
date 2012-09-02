#!/usr/bin/perl

$m{'TAB'} = 'Tab';
$m{'SPC'} = 'space';
$m{'DEL'} = 'Delete';
$m{'RET'} = 'Return';
$m{'ESC'} = 'Escape';
$m{'UP'} = 'Up';
$m{'DOWN'} = 'Down';
$m{'LEFT'} = 'Left';
$m{'RIGHT'} = 'Right';
$m{'INS'} = 'Insert';

open IN, $ARGV[0];
while (<IN>) {
    chomp;
    ($k, $c, @a) = split /\s+/;

    $comment = 0;
    if ($k =~ s/^\#//) {
        $comment = 1;
    }

    if ($k eq '') {
        if ($comment) {
            print "/* $_ */\n";
        }
        else {
            print "\n";
        }
        next;
    }

    $shift = 0;
    $alt = 0;
    $ctrl = 0;
    while (1) {
        if ($k =~ s/^S-//) {
            $shift = 1;
        }
        elsif ($k =~ s/^M-//) {
            $alt = 1;
        }
        elsif ($k =~ s/^C-//) {
            $ctrl = 1;
        }
        last;
    }

    if ($m{$k}) {
        $k = "XK_" . $m{$k};
    }
    elsif ($k =~ /^BUTTON(\d)/) {
        $k = "XK_Pointer_Button$1";
    }
    else {
        $k = "XK_" . $k;
    }

    if ($shift) {
        $k .= "|SHIFT";
    }
    if ($ctrl) {
        $k .= "|CTRL";
    }
    if ($alt) {
        $k .= "|ALT";
    }

    if ($c eq 'KEY_MARK') {
        $c = 'ev_mark_client';
    }
    else {
        $c = "ev_" . lc($c);
    }

    $args = join(' ', @a);

    $args = "\"$args\"";

    $line = "KEY($k, $c, $args)";

    if ($comment) {
        $line = "/* $line */";
    }

    print $line . "\n";
}
close IN;

