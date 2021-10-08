#!/usr/bin/perl

use strict;

my $i; my $findResult;
my $INSTALL_PREFIX="/tmp/fceux";

if ( $#ARGV >= 0 )
{
   $INSTALL_PREFIX=$ARGV[0];
}

if ( ! -d "$INSTALL_PREFIX")
{
   die "Error Invalid install prefix: $INSTALL_PREFIX\n";
}

print "INSTALL PREFIX: $INSTALL_PREFIX\n";

$findResult = `find $INSTALL_PREFIX -d -name fceux.app`;

if ( $findResult ne "")
{
   $findResult =~ s/\n.*//;
   $INSTALL_PREFIX=$findResult;
}
else
{
   $INSTALL_PREFIX="$INSTALL_PREFIX/fceux.app";
}
print "INSTALL PREFIX: $INSTALL_PREFIX\n";

# MacOSX homebrew version of ffmpeg comes with 5 libraries. These libraries depend on each other
# and it seems that macdeployqt doesn't fix all the library paths when bundling them.
# This script fixes those bundling issues.
my $LIBPATH="$INSTALL_PREFIX/Contents/Frameworks";
#my @libList = ( "libavutil", "libavcodec", "libavformat", "libswscale", "libswresample" );
my $lsList = `ls $LIBPATH/*.dylib`;
my @libList = split /\n/, $lsList;
my $lib;
my %libsDone;

for ($i=0; $i<=$#libList; $i++)
{
    $lib="$libList[$i]";
    $lib=~s/\n//;
#
    fixLib($lib);
}

sub fixLib
{
   my $j;
   my $lib = $_[0];
   my $otool;
   my $depPath; my $depName;
   my @lines;
   my $cmd;

   if ( defined($libsDone{$lib}))
   {
      #print "Lib Done: $lib\n";
      return;
   }
    print "CHECKING LIB DEPS: '$libList[$i]'\n";
#
   $libsDone{$lib} = 1;
   #print "Checking lib: '$lib'\n";

   $otool=`otool -L $lib`;

   #print "otool: '$otool'\n";

   @lines = split /\n/, $otool;

   for ($j=1; $j<=$#lines; $j++)
   {
      if ( $lines[$j] =~ m/\s+(\/usr\/local\/.*(lib.*\.dylib))/ )
      {
         $depPath = $1;
         $depName = $2;
         #print "$1  $2\n";

         if (-e "$LIBPATH/$depName")
         {
            #print "Found Packaged $depName...\n";

            $cmd = "install_name_tool  -change  $depPath  \@executable_path/../Frameworks/$depName  $lib";
            print "\tFIXING LIB LINK: '$depPath'\n";
            #print("$cmd\n");
            system($cmd);
         }
      }
   }
}
