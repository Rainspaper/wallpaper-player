QT       += core gui widgets

CONFIG   += c++17
TEMPLATE  = app
TARGET    = WallpaperPlayer

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/directoryscanner.cpp \
    src/filterpanel.cpp \
    src/wallpapercarddelegate.cpp

HEADERS += \
    src/wallpaperdata.h \
    src/mainwindow.h \
    src/directoryscanner.h \
    src/filterpanel.h \
    src/wallpapercarddelegate.h

RESOURCES += resources.qrc

RC_ICONS = ico1.ico
