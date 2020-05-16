#!/usr/bin/perl

use strict;

my $VERSION="2.2.4";
my $INSTALL_PREFIX="/tmp/fceux";
my $CTL_FILENAME="$INSTALL_PREFIX/DEBIAN/control";

my $SO_LIST=`objdump -x  $INSTALL_PREFIX/usr/bin/fceux`;

#print "$SO_LIST";

my $i; my $j; my $k; my $PKG;
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

   #$PKG=`dpkg-query -S  $fd[$i] | cut -d ':' -f1`;
}

my @rpm;
my @rpmfile;
my @rpmfilelist;
my $rpmlist = `rpm -qa`;
@rpm = split /\n/, $rpmlist;

$#rpmfilelist = 0;

for ($i=0; $i<=$#rpm; $i++)
{
   #print "$fd[$i]\n";
   #$pkg = `rpm -qf   --queryformat '%{NAME}'`;
   #
   $rpmfilelist[ $#rpmfilelist ] = `rpm -ql $rpm[$i]`;
   $#rpmfilelist++;
}

for ($i=0; $i<$#libls; $i++)
{
   for ($j=0; $j<$#rpmfilelist; $j++)
   {
      @rpmfile = split /\n/, $rpmfilelist[$j];

      for ($k=0; $k<=$#rpmfile; $k++)
      {
         if ( $rpmfile[$k] =~ m/$libls[$i]/ )
         {
            print "FOUND: $rpmfile[$k] in $rpm[$j]\n";
         }
      }
   }
}
#for LIB in $LDD_LIST
#do
#   PKG+=`dpkg-query -S  $LIB | cut -d ':' -f1`;
#done
#
#
system("mkdir -p $INSTALL_PREFIX/DEBIAN");

open CTL, ">$CTL_FILENAME" or die "Error: Could not open file '$CTL_FILENAME'\n";
#
print CTL "Package: fceux";
print CTL "Version: $VERSION";
print CTL "Section: games";
print CTL "Priority: extra";
print CTL "Architecture: amd64";
print CTL "Homepage: http://fceux.com/";
print CTL "Essential: no";
print CTL "Installed-Size: 1024";
print CTL "Maintainer: mjbudd77";
print CTL "Description: fceux is an emulator of the original (8-bit) Nintendo Entertainment System (NES)";
print CTL "Depends: liblua5.1-0, libsdl1.2debian, libsdl2-2.0-0, libminizip1, libgtk-3-0 ";

close CTL;
#
#cd /tmp;
#dpkg-deb  --build  fceux 
#mv  fceux.deb  fceux-$VERSION.deb
#dpkg-deb  -I   fceux-$VERSION.deb
#
