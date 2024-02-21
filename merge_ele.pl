#!/usr/bin/env perl

# On a bien 2 args ?
if ($#ARGV != 1) {
    print "usage: <GPX original> <GPX with elevation>\n";
    exit;
}

open(FILE_ORG, $ARGV[0]) || die "open error: '$ARGV[0]' - $!\n";
open(FILE_ELE, $ARGV[1]) || die "open error: '$ARGV[1]' - $!\n";

while (<FILE_ORG>) {
	$ln_org = $_;
	printf("%s", $ln_org);
	if ($ln_org =~ m/^\s*<trkpt /i) {
		while (<FILE_ELE>) {
			$ln_ele = $_;
			if ($ln_ele =~ m/^\s*<ele>/i) {
				printf("%s", $ln_ele);
				last;
			}
		}
	}
}

close(FILE_ORG) || warn "close failed: '$ARGV[0]' - $!\n";
close(FILE_ELE) || warn "close failed: '$ARGV[1]' - $!\n";

exit;

