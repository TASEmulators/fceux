#!/usr/bin/perl

use strict;
use File::Basename;
#use File::Spec;

# 
# Global Variables
#
my $platform = "";
my $fceuVersionMajor = 1;
my $fceuVersionMinor = 0;
my $fceuVersionPatch = 0;

foreach my $arg (@ARGV)
{
	#print $arg, "\n";

	if ($platform eq "")
	{
		$platform = $arg;
	}
}

my $dirname = dirname(__FILE__);
my $projRoot = "$dirname/..";
my $ReleaseBuild=0;
my $ReleaseVersion="";

#print "PATH: $ENV{PATH}\n";
#print "Dir $dirname\n";
#
($fceuVersionMajor, $fceuVersionMinor, $fceuVersionPatch) = getVersion();

($ReleaseBuild, $ReleaseVersion) = isReleaseBuild();

if ($ReleaseBuild)
{
	$ENV{FCEU_RELEASE_VERSION} = $ReleaseVersion;
}

if ($platform eq "win32")
{
	build_win32();
}
elsif ($platform eq "win64")
{
	build_win64();
}
elsif ($platform eq "win64-QtSDL")
{
	build_win64_QtSDL();
}
elsif ($platform eq "linux")
{
	build_ubuntu_linux();
}
elsif ($platform eq "macOS")
{
	build_macOS();
}
#--------------------------------------------------------------------------------------------
# Build win32 version
#--------------------------------------------------------------------------------------------
sub build_win32
{
	chdir("$projRoot");

	my $ret = system("cmd.exe /c pipelines\\\\win32_build.bat");

	if ($ret != 0){ die "Build Errors Detected\n";}
}
#--------------------------------------------------------------------------------------------
# Build win64 version
#--------------------------------------------------------------------------------------------
sub build_win64
{
	chdir("$projRoot");

	my $ret = system("cmd.exe /c pipelines\\\\win64_build.bat");

	if ($ret != 0){ die "Build Errors Detected\n";}
}
#--------------------------------------------------------------------------------------------
# Build win64-Qt/SDL version
#--------------------------------------------------------------------------------------------
sub build_win64_QtSDL
{
	chdir("$projRoot");

	my $ret = system("cmd.exe /c pipelines\\\\qwin64_build.bat");

	if ($ret != 0){ die "Build Errors Detected\n";}
}
#--------------------------------------------------------------------------------------------
# Build Ubuntu Linux version
#--------------------------------------------------------------------------------------------
sub build_ubuntu_linux
{
	chdir("$projRoot");

	my $ret = system("./pipelines/linux_build.sh");

	if ($ret != 0){ die "Build Errors Detected\n";}
}
#--------------------------------------------------------------------------------------------
# Build MacOSX version
#--------------------------------------------------------------------------------------------
sub build_macOS
{
	chdir("$projRoot");

	my $ret = system("./pipelines/macOS_build.sh");

	if ($ret != 0){ die "Build Errors Detected\n";}
}
#--------------------------------------------------------------------------------------------
# Search src/version.h and retrieve version numbers
#--------------------------------------------------------------------------------------------
sub getVersion
{
	my $versionHeader = "$projRoot/src/version.h";
	my $line;
	my $major = 1;
	my $minor = 0;
	my $patch = 0;
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

	return ( $major, $minor, $patch );
}
#--------------------------------------------------------------------------------------------
# Returns whether this is a release build and returns the version if detected
#--------------------------------------------------------------------------------------------
sub isReleaseBuild
{
	my $isRelease = 0;
	my $tagVersion = "";

	if (defined($ENV{APPVEYOR_REPO_TAG_NAME}))
	{
		if ($ENV{APPVEYOR_REPO_TAG_NAME} =~ m/fceux-(\d+\.\d+\.\d+)/)
		{
			$tagVersion = $1;
			$isRelease = 1;
		}
		elsif ($ENV{APPVEYOR_REPO_TAG_NAME} =~ m/(\d+\.\d+\.\d+)/)
		{
			$tagVersion = $1;
			$isRelease = 1;
		}
	}
	return ($isRelease, $tagVersion);
}
