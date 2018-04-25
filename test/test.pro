TARGET  = test
TEMPLATE = app
QT      += core
LIBS    += -ldynes
include(../common.pri)

###
INCLUDEPATH += include

SOURCES += \
    src/bmpfile.cpp \
    src/casetteplayer.cpp \
    src/displaystore.cpp \
    src/instructionexecutor.cpp \
    src/main.cpp

HEADERS += \
    include/bmpfile.hpp \
    include/casetteplayer.hpp \
    include/displaystore.hpp \
    include/instructionexecutor.hpp
