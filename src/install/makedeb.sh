#!/bin/bash

arch=$1
ver=$2
appname="a1menu"
cfgapp="a1menu.run"
exebase="/D/c++/qtworkspace/Applet/"
debbase="/tmp/a1menu-deb"

if [ "$arch" != "amd64" ] && [ "$arch" != "i386" ] ; then
   echo "Missing architecture: amd64 or i386 expected"
   exit
fi

if [ "$ver" == "" ]  ; then
   echo "Missing version"
   exit
fi


if [ "$arch" == "amd64" ] ; then
   echo "Warning: installing amd64 debug version"
   exedirinst="$exebase/Release/"
else
   exedirinst="$exebase/Release32/"
fi

exepath="$exedirinst/$cfgapp"


function xcmptime
{
if test $exebase/$1 -nt $exepath; then
   echo  "$1 is newer than $exepath"
   exit
fi
}

xcmptime "Debug/$cfgapp"
xcmptime "Debug32/$cfgapp"
xcmptime "Release/$cfgapp"
xcmptime "Release32/$cfgapp"

rm -r $debbase/*

debpkg="$appname-${arch}-${ver}"
debdirD="$debbase/$debpkg"
debdirDD=$debdirD/DEBIAN


cfgser="org.mate.panel.applet.a1menu.service"
#cfgxml="org.mate.panel.applet.a1menu.gschema.xml"
cfgpan="org.mate.applets.a1menu.mate-panel-applet"

dirser="usr/share/dbus-1/services"
#dirxml="usr/share/glib-2.0/schemas"
dirpan="usr/share/mate-panel/applets"
dirapp="usr/lib/mate-applets"
dirtmp="tmp/$appname"

mkdir -p $debdirDD
fsum="$debdirDD/md5sums"

echo "" > $fsum

cwd=$(pwd)
dirinst=$cwd

function xcp 
{
  mkdir -p  $debdirD/$1
  cp  $dirinst/$2 $debdirD/$1/$2
  cd $debdirD
  md5sum $1/$2 >> $fsum
  cd $cwd
}  

cp ./control-$arch  $debdirDD/control
cp ./postinst $debdirDD

xcp $dirser $cfgser
#xcp $dirxml $cfgxml
xcp $dirpan $cfgpan

dirinst=$exedirinst

xcp $dirapp $cfgapp

dpkg-deb --build $debdirD

