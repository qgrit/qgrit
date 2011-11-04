QT      += core gui

DEFINES += QT_NO_CAST_FROM_ASCII QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_BYTEARRAY QT_STRICT_ITERATORS

TARGET = qgrit
TEMPLATE = app


SOURCES += \
    qgrit.cpp \
    rebasedialog.cpp \
    commandcombo.cpp \
    configdialog.cpp \
    gittool.cpp

HEADERS += \
    rebasedialog.h \
    commandcombo.h \
    configdialog.h \
    gittool.h

FORMS   += \
    rebasedialog.ui \
    configdialog.ui

win32 {
    RC_FILE = icon.rc
}

OTHER_FILES += \
    README.txt

RESOURCES += \
    qgrit.qrc
