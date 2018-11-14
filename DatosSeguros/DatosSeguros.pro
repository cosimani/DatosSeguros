#-------------------------------------------------
#
# Project created by QtCreator 2018-03-16T11:24:10
#
#-------------------------------------------------

QT       += core gui  

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = app

CONFIG += c++11

###########################################################################################
unix:DIR_OPENCV_LIBS = /usr/local/lib  ####################################################
###########################################################################################

unix:LIBS += $$DIR_OPENCV_LIBS/libopencv_core.so
unix:LIBS += $$DIR_OPENCV_LIBS/libopencv_highgui.so
unix:LIBS += $$DIR_OPENCV_LIBS/libopencv_video.so
unix:LIBS += $$DIR_OPENCV_LIBS/libopencv_imgproc.so
unix:LIBS += $$DIR_OPENCV_LIBS/libopencv_objdetect.so
unix:LIBS += $$DIR_OPENCV_LIBS/libopencv_videoio.so
unix:LIBS += $$DIR_OPENCV_LIBS/libopencv_dnn.so
unix:LIBS += $$DIR_OPENCV_LIBS/libopencv_imgcodecs.so
unix:LIBS += $$DIR_OPENCV_LIBS/libopencv_features2d.so
unix:LIBS += $$DIR_OPENCV_LIBS/libopencv_calib3d.so

unix:LIBS += $$DIR_OPENCV_LIBS/libtesseract.so
unix:LIBS += $$DIR_OPENCV_LIBS/libleptonica.so



SOURCES += \
    main.cpp \
    visor.cpp \ 
    principal.cpp \
    procesador.cpp

HEADERS += \
    visor.h \ 
    principal.h \
    procesador.h

FORMS += \
    principal.ui

