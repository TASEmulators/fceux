#!/usr/bin/perl

use strict;
use File::Basename;
#use File::Spec;

my $format = 0;

foreach my $arg (@ARGV)
{
	#print $arg, "\n";

    if ($arg eq "-major")
    {
	$format = 1;
    }
    elsif ($arg eq "-minor")
    {
	$format = 2;
    }
    elsif ($arg eq "-patch")
    {
	$format = 3;
    }
}

#my $file = File::Spec->rel2abs( __FILE__ );
my $dirname = dirname(__FILE__);
my $projRoot = "$dirname/..";
my $versionHeader = "$projRoot/src/version.h";
my $major = 1;
my $minor = 0;
my $patch = 0;

#print "File: $file\n";
#print "Dir $dirname\n";

my $line;

open INFILE, "$versionHeader" or die "Error: Could not open file: $versionHeader\n";
while ($line = <INFILE>)
{
	#print $line;
	if ($line =~ m/\s*#define\s+FCEU_VERSION_MAJOR\s+(\d+)/)
	{
		$major = $1;
	}
	elsif ($line =~ m/\s*#define\s+FCEU_VERSION_MINOR\s+(\d+)/)
	{
		$minor = $1;
	}
	elsif ($line =~ m/\s*#define\s+FCEU_VERSION_PATCH\s+(\d+)/)
	{
		$patch = $1;
	}
}
close(INFILE);

if ($format == 1)
{
	print "$major";
}
elsif ($format == 2)
{
	print "$minor";
}
elsif ($format == 3)
{
	print "$patch";
}
else
{
	print "$major.$minor.$patch";
}
