#!/bin/bash

fname="a1menu-gtk"

loc="pl"

sub="./$loc/LC_MESSAGES"

mkdir -p $sub

xgettext --keyword=_ --language=C --add-comments --sort-output -o $fname.pot ../*.h ../*.cpp

#msginit --input=./$fname.pot --locale=$loc --output=$sub/$fname.po
msgmerge --output-file=$sub/$fname.po    $sub/$fname.po  ./$fname.pot
msgfmt --output-file=$sub/$fname.mo $sub/$fname.po

dstdir=$HOME/.a1menu-gtk/po/$sub
mkdir -p $dstdir

cp $sub/*.mo $dstdir