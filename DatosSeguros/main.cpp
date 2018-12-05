//#include <QApplication>
#include <QCoreApplication>
#include "primeraentrega.h"
#include "cliente.h"

#include "procesador.h"

#include "config.h"
#include "webservice.h"
#include <QFile>


int main(int argc, char *argv[])
{
//    QApplication a(argc, argv);
    QCoreApplication a(argc, argv);

    // No se muy bien que hace esto, pero es para que no salte:
    // strcmp(locale, "C"):Error:Assert failed:in file /home/cosimani/Proyecto/DatosSeguros/tesseract-ocr/src/api/baseapi.cpp
    // Al querer instanciar un objeto de tesseract::TessBaseAPI
    setlocale( LC_ALL, "C" );

//    Procesador::getInstancia()->configurarImageAlignment( Procesador::LICENCIA,
//                                                          "../../imagenes/referencias/LicenciaFrente.jpg" );

#ifdef EJECUTADO_EN_SERVER

    Procesador::getInstancia()->configurarImageAlignment( Procesador::LICENCIA,
                                                          "imagenes/referencias/LicenciaFrente.jpg" );

    Procesador::getInstancia()->configurarImageAlignment( Procesador::DNI,
                                                          "imagenes/referencias/DniFrente.jpg" );


#else

    Procesador::getInstancia()->configurarImageAlignment( Procesador::LICENCIA,
                                                          "../../imagenes/referencias/LicenciaFrente.jpg" );

    Procesador::getInstancia()->configurarImageAlignment( Procesador::DNI,
                                                          "../../imagenes/referencias/DniFrente.jpg" );
#endif


    QFile file;
    file.setFileName("../../imagenes/referencias/DniFrente.jpg" );
    qDebug() << "../../imagenes/referencias/DniFrente.jpg" << file.exists();
    file.setFileName("../../imagenes/referencias/LicenciaFrente.jpg" );
    qDebug() << "../../imagenes/referencias/LicenciaFrente.jpg" << file.exists();


    WebService * webService = new WebService;

    webService->iniciar( Config::getInstance()->getString( "puerto_tcp" ).toInt() );


//    PrimeraEntrega principal;
//    principal.resize( 1050, 660 );
//    principal.show();



    return a.exec();
}



