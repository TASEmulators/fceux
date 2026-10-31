#!/usr/bin/perl

use strict;
use warnings;
use File::Basename qw(dirname basename);

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

my $findResult = `find "$INSTALL_PREFIX" -d -name fceux.app`;

if ( $findResult ne "")
{
   $findResult =~ s/\n.*//s;
   $INSTALL_PREFIX=$findResult;
}
else
{
   $INSTALL_PREFIX="$INSTALL_PREFIX/fceux.app";
}
print "INSTALL PREFIX: $INSTALL_PREFIX\n";

# The Homebrew version of ffmpeg (and libarchive, etc.) references its own dependencies
# using paths relative to the Homebrew cellar, such as:
#   @loader_path/../../../../opt/zstd/lib/libzstd.1.dylib
# macdeployqt does not follow these references, so the dependent libraries never make
# it into the app bundle and the app aborts at launch with "Library not loaded".
# This script finds every such dependency, copies it into the bundle, and rewrites the
# install names so that the bundle is self contained.
my $LIBPATH="$INSTALL_PREFIX/Contents/Frameworks";
my $FWREF='@executable_path/../Frameworks';

my @searchRoots = brewPrefixes();
my @queue = findMachOFiles($INSTALL_PREFIX);
my %seen = map { $_ => 1 } @queue;
my $file;

while ( defined( $file = shift @queue ) )
{
   fixFile($file);
}

verifyBundle();

system('codesign', '--force', '--deep', '--sign', '-', $INSTALL_PREFIX) == 0
   or die "Failed to re-sign app bundle: $INSTALL_PREFIX\n";

sub brewPrefixes
{
   my @roots;

   if ( defined($ENV{HOMEBREW_PREFIX}) && $ENV{HOMEBREW_PREFIX} ne "" )
   {
      push @roots, $ENV{HOMEBREW_PREFIX};
   }
   my $prefix = `brew --prefix 2>/dev/null`;
   chomp $prefix;
   if ( $prefix ne "" )
   {
      push @roots, $prefix;
   }
   push @roots, "/opt/homebrew", "/usr/local";

   my %uniq;
   return grep { -d $_ && !$uniq{$_}++ } @roots;
}

# Returns every Mach-O file (executables, dylibs, framework binaries, plugins) in the bundle
sub findMachOFiles
{
   my $app = $_[0];
   my @result;

   foreach my $dir ( "Contents/MacOS", "Contents/Frameworks", "Contents/PlugIns" )
   {
      next if ( ! -d "$app/$dir" );

      open( my $fh, '-|', 'find', "$app/$dir", '-type', 'f' ) or die "Failed to run find: $!\n";
      while ( my $path = <$fh> )
      {
         chomp $path;
         my $desc = `file -b "$path"`;
         push @result, $path if ( $desc =~ m/Mach-O/ );
      }
      close($fh);
   }
   return @result;
}

sub getDeps
{
   my $file = $_[0];
   my @lines = split /\n/, `otool -L "$file"`;
   my @idLines = split /\n/, `otool -D "$file"`;
   my $id = $idLines[1];  # Only present for dylibs and frameworks
   my @deps;

   shift @lines;  # First line is the file name
   foreach my $line (@lines)
   {
      if ( $line =~ m/^\s+(\S.*?)\s+\(compatibility version/ )
      {
         if ( defined($id) && $1 eq $id )
         {
            $id = undef;  # Skip the library's own install name
            next;
         }
         push @deps, $1;
      }
   }
   return @deps;
}

sub isSystemOrResolved
{
   my ($file, $dep) = @_;

   return 1 if ( $dep =~ m{^/usr/lib/} || $dep =~ m{^/System/} );
   return 1 if ( $dep =~ m{^\@rpath/} );  # Qt frameworks, handled by macdeployqt
   return 1 if ( index($dep, "$FWREF/") == 0 );

   if ( $dep =~ m{^\@loader_path/(.*)$} )
   {
      my $local = dirname($file) . "/$1";
      return 1 if ( -e $local );
   }
   return 0;
}

# Locate the real library on the build machine that corresponds to a dependency reference
sub resolveSource
{
   my $dep = $_[0];
   my $name = basename($dep);
   my @candidates;

   if ( $dep =~ m{^/} )
   {
      push @candidates, $dep;
   }

   foreach my $root (@searchRoots)
   {
      if ( $dep =~ m{/opt/([^/]+)/(lib/.+)$} )
      {
         push @candidates, "$root/opt/$1/$2";
      }
      if ( $dep =~ m{/Cellar/([^/]+)/[^/]+/(lib/.+)$} )
      {
         push @candidates, "$root/opt/$1/$2";
      }
      push @candidates, "$root/lib/$name";
      push @candidates, glob("$root/opt/*/lib/$name");
   }

   foreach my $cand (@candidates)
   {
      return $cand if ( -e $cand );
   }
   return undef;
}

sub fixFile
{
   my $file = $_[0];

   print "CHECKING LIB DEPS: '$file'\n";

   # Libraries copied in by macdeployqt may still carry their original install name
   my @idLines = split /\n/, `otool -D "$file"`;
   if ( index($file, "$LIBPATH/") == 0 && defined($idLines[1]) && $idLines[1] !~ m{^\@(rpath|executable_path|loader_path)/} )
   {
      system('chmod', 'u+w', $file);
      system('install_name_tool', '-id', "$FWREF/" . basename($file), $file) == 0
         or die "Failed to set install name of $file\n";
   }

   foreach my $dep ( getDeps($file) )
   {
      next if ( isSystemOrResolved($file, $dep) );

      my $name = basename($dep);
      my $dest = "$LIBPATH/$name";

      if ( ! -e $dest )
      {
         my $src = resolveSource($dep);

         if ( !defined($src) )
         {
            die "Failed to locate dependency '$dep' of '$file'\n";
         }
         print "\tBUNDLING LIB: '$src'\n";

         system('mkdir', '-p', $LIBPATH) == 0 or die "Failed to create $LIBPATH\n";
         system('cp', '-L', $src, $dest) == 0 or die "Failed to copy $src\n";
         system('chmod', 'u+w', $dest) == 0 or die "Failed to chmod $dest\n";
         system('install_name_tool', '-id', "$FWREF/$name", $dest) == 0
            or die "Failed to set install name of $dest\n";
      }
      if ( !$seen{$dest} )
      {
         $seen{$dest} = 1;
         push @queue, $dest;
      }

      print "\tFIXING LIB LINK: '$dep'\n";
      system('install_name_tool', '-change', $dep, "$FWREF/$name", $file) == 0
         or die "Failed to fix library dependency: $dep\n";
   }
}

# Fail the build if any library still references something outside of the bundle
sub verifyBundle
{
   my $bad = 0;

   foreach my $file ( findMachOFiles($INSTALL_PREFIX) )
   {
      foreach my $dep ( getDeps($file) )
      {
         next if ( isSystemOrResolved($file, $dep) );

         print "ERROR: '$file' has unresolved dependency '$dep'\n";
         $bad = 1;
      }
   }
   die "App bundle is not self contained\n" if ( $bad );
}
