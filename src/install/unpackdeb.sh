
tmpdir=/tmp/

debpkg=$(readlink -f $1)

rm -rf $tmpdir/unpack
mkdir $tmpdir/unpack
cd $tmpdir/unpack
ar -x $debpkg
mkdir ./deb
cd deb
tar -xf ../data.tar.xz


for f in $(find . -type f)
do
  echo "Copy $f"
  cp $f /$f
  if [ $? -eq 0 ]; then
    echo OK
  else
    echo FAIL
  fi
done

glib-compile-schemas /usr/share/glib-2.0/schemas


