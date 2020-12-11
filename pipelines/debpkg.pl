#!/usr/bin/perl

use strict;

my $VERSION="2.3.0";
my $INSTALL_PREFIX="/tmp/fceux";
my $CTL_FILENAME="$INSTALL_PREFIX/DEBIAN/control";
my $ARCH="amd64";
my $PKG_OUTPUT_FILE="fceux-$VERSION-$ARCH.deb";

# Start by auto figuring out dependencies of the executable.
# the rest of the package creation is trival.
my $SO_LIST="";
$SO_LIST=`objdump -x  $INSTALL_PREFIX/usr/bin/fceux`;
$SO_LIST= $SO_LIST . `objdump -x  $INSTALL_PREFIX/usr/bin/fceux-gtk`;

#print "$SO_LIST";

my $i; my $j; my $k; my $pkg;
my @fd = split /\n/, $SO_LIST;
my @libls;

$#libls=0;

for ($i=0; $i<=$#fd; $i++)
{
   #$fd[$i] =~ s/^\s+//;
   #print "$fd[$i]\n";

   if ( $fd[$i] =~ m/NEEDED\s+(.*)/ )
   {
      #print "$1 \n";
      $libls[$#libls] = $1; $#libls++;
   }

}

my %pkghash; my $pkgsearch;
my @pkglist; my @pkgdeps;
my $pkg; my $filepath;

$#pkgdeps=0;

for ($i=0; $i<$#libls; $i++)
{
   $pkgsearch=`dpkg-query -S  $libls[$i]`;

   @pkglist = split /\n/, $pkgsearch;

   for ($j=0; $j<=$#pkglist; $j++)
   {
      #$pkghash{$pkg} = 1;
      #print "  $libls[$i]    '$pkglist[$j]'  \n";

      if ( $pkglist[$j] =~ m/(.*):$ARCH:\s+(.*)/ )
      {
	       $pkg = $1;
          $filepath = $2;
          
          $filepath =~ s/^.*\///;
          
	       if ( $libls[$i] eq $filepath )
	       {
             #print "PKG:   '$pkg'   '$libls[$i]' ==  '$filepath' \n";
             # Don't put duplicate entries into the pkg depend list.
             if ( !defined( $pkghash{$pkg} ) )
             {
                $pkgdeps[ $#pkgdeps ] = $pkg; $#pkgdeps++;
                $pkghash{$pkg} = 1;
             }
          }
      }
   }
}
#
#
system("mkdir -p $INSTALL_PREFIX/DEBIAN");

open CTL, ">$CTL_FILENAME" or die "Error: Could not open file '$CTL_FILENAME'\n";
#
print CTL "Package: fceux\n";
print CTL "Version: $VERSION\n";
print CTL "Section: games\n";
print CTL "Priority: extra\n";
print CTL "Architecture: $ARCH\n";
print CTL "Homepage: http://fceux.com/\n";
print CTL "Essential: no\n";
#print CTL "Installed-Size: 1024\n";
print CTL "Maintainer: mjbudd77\n";
print CTL "Description: fceux is an emulator of the original (8-bit) Nintendo Entertainment System (NES)\n";
print CTL "Depends: $pkgdeps[0]";

for ($i=1; $i<$#pkgdeps; $i++)
{
   print CTL ", $pkgdeps[$i]";
}
print CTL "\n";

close CTL;

#system("cat $CTL_FILENAME");
#
chdir "/tmp";
system("dpkg-deb  --build  fceux ");
if ( !(-e "/tmp/fceux.deb") )
{ 
   die "Error: Failed to create package $PKG_OUTPUT_FILE\n";
}
system("mv  fceux.deb  $PKG_OUTPUT_FILE");
system("dpkg-deb  -I   $PKG_OUTPUT_FILE");
#
if ( -e "/tmp/$PKG_OUTPUT_FILE" )
{ 
   print "**********************************************\n";
   print "Created deb package: /tmp/$PKG_OUTPUT_FILE\n";
   print "**********************************************\n";
}
else
{
   die "Error: Failed to create package $PKG_OUTPUT_FILE\n";
}
