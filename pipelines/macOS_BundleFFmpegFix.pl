#!/usr/bin/perl

use strict;

# MacOSX homebrew version of ffmpeg comes with 5 libraries. These libraries depend on each other
# and it seems that macdeployqt doesn't fix all the library paths when bundling them.
# This script fixes those bundling issues.
my $i; my $j; my $k;
my $LIBPATH=`brew --prefix ffmpeg`;
my @libList = ( "libavutil", "libavcodec", "libavformat", "libswscale", "libswresample" );
my $lib; my $otool;
my @lines;

for ($i=0; $i<=$#libList; $i++)
{
   #print "LIB: $libList[$i]\n";

   $lib="$LIBPATH/lib/$libList[$i].dylib";
   $lib=~s/\n//;

   print "Fixing lib: '$lib'\n";

   $otool=`otool -L $lib`;

   #print "otool: '$otool'\n";

   @lines = split /\n/, $otool;

   for ($j=0; $j<=$#lines; $j++)
   {
      for ($k=0; $k<=$#libList; $k++)
      {
         if ( $lines[$j] =~ m/\s+(\/.*\/($libList[$k].*\.dylib))/ )
         {
            #print "$1  $2\n";

            print("install_name_tool  -change  $1  \@executable_path/../Frameworks/$2  $lib\n");
            system("install_name_tool  -change  $1  \@executable_path/../Frameworks/$2  $lib");
         }
      }
   }
}
