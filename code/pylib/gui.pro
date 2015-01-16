TEMPLATE = app
TARGET = gui
QT += opengl widgets
LIBS += -lIL -lILU -lGLU -lCGAL -lgmp -lboost_thread
INCLUDEPATH += .
QMAKE_CXXFLAGS += -frounding-math

# Input
HEADERS += algo.h \
           coord.h \
           drawing.h \
           drawwidget.h \
           edge.h \
           gl.h \
           il.h \
           image.h \
           neighbours.h \
           opengldrawwidget.h \
           polygon.h \
           room.h \
           roomimage.h \
           texture.h \
           triangle.h \
           triangulation.h \
           widgets.h
SOURCES += algo.cpp \
           coord.cpp \
           drawing.cpp \
           drawwidget.cpp \
           edge.cpp \
           gl.cpp \
           il.cpp \
           image.cpp \
           main.cpp \
           opengldrawwidget.cpp \
           polygon.cpp \
           room.cpp \
           roomimage.cpp \
           texture.cpp \
           triangle.cpp \
           triangulation.cpp \
           widgets.cpp
