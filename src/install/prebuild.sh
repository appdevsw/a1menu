#!/bin/bash

exepath=/D/c++/qtworkspace/Applet/Debug
srcpath=/D/c++/qtworkspace/Applet/qt-lnk/a1menu

resname=a1menu.res.gz
respath=/tmp/$resname
hheader=$srcpath/$resname.h

$($exepath/a1menu.run --crres $srcpath $respath)

h1=$(md5sum $hheader)
xxd -i  $respath > $hheader
h2=$(md5sum $hheader)


echo $h1
echo $h2

if [ "$h1" != "$h2" ] ; then

  echo "$src was changed. Rebuild required."

  objpath="/D/c++/qtworkspace/Applet/Debug/qt-lnk/a1menu/"
  exepath="/D/c++/qtworkspace/Applet/Debug/"
  
  frun="$exepath/a1menu.run"
  touch $frun
  rm $frun
  
  fobj="$objpath/config.o"
  touch $fobj
  rm $fobj
 
  #echo "Prebuild: copy $src to ./.a1menu"
  #cp $src ~/.a1menu/
fi

