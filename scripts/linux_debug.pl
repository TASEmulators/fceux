#!/usr/bin/perl

use strict;

my $i; my $findResult;
my $exe = "fceux";

$findResult = `find . -name fceux`;

if ( $findResult ne "")
{
   $findResult =~ s/\n.*//;
   $exe=$findResult;
}
print "Executable: $exe\n";

my $gdbCmdFile = "/tmp/gdbCmdFile";
open CMD_FILE, ">$gdbCmdFile" or die "Error: Could not open file: $gdbCmdFile\n";
print CMD_FILE "run\n";
print CMD_FILE "list\n";
print CMD_FILE "backtrace\n";
close(CMD_FILE);

system("gdb  -x $gdbCmdFile  $exe");

