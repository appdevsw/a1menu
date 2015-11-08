
QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = a1menu.run
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    item.cpp \
    itemlist.cpp \
    toolkit.cpp \
    userevent.cpp \
    itemproperties.cpp \
    configmap.cpp \
    loader/applicationloader.cpp \
    loader/categoryloader.cpp \
    loader/iconloader.cpp \
    mainapplet.cpp \
    config.cpp \
    ctx.cpp \
    itemlabel.cpp \
    searchbox.cpp \
    toolbutton.cpp \
    parameterform.cpp \
    imgconv.cpp \
    x11util.cpp \
    loader/loader.cpp \
    resource.cpp

QMAKE_CXXFLAGS += -std=c++11 `pkg-config --cflags --libs gtk+-2.0 x11`

HEADERS  += mainwindow.h \
    item.h \
    itemlist.h \
    toolkit.h \
    userevent.h \
    itemproperties.h \
    configmap.h \
    loader/applicationloader.h \
    loader/categoryloader.h \
    loader/iconloader.h \
    config.h \
    ctx.h \
    itemlabel.h \
    searchbox.h \
    toolbutton.h \
    parameterform.h \
    imgconv.h \
    x11util.h \
    loader/loader.h \
    resource.h

FORMS    += mainwindow.ui \
    item.ui \
    itemlist.ui \
    itemproperties.ui \
    parameterform.ui

INCLUDEPATH += /usr/include/qt4/QtXml \
    ./loader \
    /D/linux/mate-dev/include/mate-desktop-master \
    /D/linux/mate-dev/include/libmate-panel-applet


QMAKE_LFLAGS = -std=c++11 `pkg-config --cflags --libs gtk+-2.0 x11`

LIBS += -L/usr/lib/x86_64-linux-gnu  \
        -lX11 \
        -lQtXml \
        -lz \
        -L/D/linux/mate-dev/lib \
        -lmate-panel-applet-4 \
        -lmate-desktop-2

TRANSLATIONS = ./ts/trans-pl_PL.ts

