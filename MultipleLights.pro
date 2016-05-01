QT += gui core

CONFIG += c++11

TARGET = MultipleLights
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    MultipleLights.cpp \
    vertex.cpp \
    vertexcol.cpp \
    vertextex.cpp \
    vbomesh.cpp \
    vboplane.cpp

HEADERS += \
    MultipleLights.h \
    vertex.h \
    vertexcol.h \
    vertextex.h \    
    vbomesh.h \
    vboplane.h

OTHER_FILES +=

RESOURCES += \
    shaders.qrc

DISTFILES += \
    fshader_2sides_multiplelights.txt \
    vshader_2sides_multiplelights.txt
