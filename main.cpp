#include <QApplication>
#include "principal.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // No se muy bien que hace esto, pero es para que no salte:
    // strcmp(locale, "C"):Error:Assert failed:in file /home/cosimani/Proyecto/DatosSeguros/tesseract-ocr/src/api/baseapi.cpp
    // Al querer instanciar un objeto de tesseract::TessBaseAPI
    setlocale( LC_ALL, "C" );

    Principal principal;
    principal.showMaximized();

    return a.exec();
}



