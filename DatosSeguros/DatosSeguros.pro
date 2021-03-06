#-------------------------------------------------
#
# Project created by QtCreator 2018-03-16T11:24:10
#
#-------------------------------------------------

#QT -= gui

QT += gui sql network

CONFIG += c++11 console
CONFIG -= app_bundle

DEFINES += EJECUTADO_EN_SERVER


#QT       += core gui sql network

#greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

#TEMPLATE = app

#CONFIG += c++11

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

unix:LIBS += $$DIR_OPENCV_LIBS/libopencv_xfeatures2d.so

unix:LIBS += $$DIR_OPENCV_LIBS/libtesseract.so
unix:LIBS += $$DIR_OPENCV_LIBS/libleptonica.so


include(QZXing/QZXing.pri)
#unix:LIBS += /home/cosimani/Proyecto/DatosSeguros/GitHub/librerias/libs/libQZXing.so.2.3.0
#INCLUDEPATH += /home/cosimani/Proyecto/DatosSeguros/GitHub/librerias/sources/zxing


SOURCES += \
    main.cpp \
    procesador.cpp \
    webservice.cpp \
    config.cpp \
    logger.cpp \
    database.cpp \
    conexion.cpp \
    validador.cpp \
    testeador.cpp

HEADERS += \
    procesador.h \
    webservice.h \
    config.h \
    logger.h \
    database.h \
    conexion.h \
    validador.h \
    testeador.h



