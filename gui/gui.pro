TARGET = Dynes
TEMPLATE = app

QT      += core gui widgets
LIBS += -ldynes
include(../common.pri)

###
INCLUDEPATH += include

SOURCES += \
  src/main.cpp \
  src/mainwindow.cpp \
  src/crtwidget.cpp \
  src/settingsdialog.cpp

HEADERS += \
  include/mainwindow.hpp \
  include/crtwidget.hpp \
  include/settingsdialog.hpp
