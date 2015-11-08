#!/bin/bash

debugPath="/D/c++/qtworkspace/Applet/Debug"

rm  /usr/share/mate-panel/applets/*a1menu*
#rm  /usr/share/glib-2.0/schemas/*a1menu*
rm  /usr/share/dbus-1/services/*a1menu*
rm  /usr/lib/mate-applets/*a1menu*



cp ./*.mate-panel-applet    /usr/share/mate-panel/applets
#cp ./*.gschema.xml          /usr/share/glib-2.0/schemas
cp ./*.service              /usr/share/dbus-1/services
cp $debugPath/a1menu.run  /usr/lib/mate-applets

glib-compile-schemas /usr/share/glib-2.0/schemas

#sudo /usr/bin/glib-compile-schemas /usr/share/glib-2.0/schemas/

