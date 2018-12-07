#include <QCoreApplication>

#include "procesador.h"
#include "config.h"
#include "logger.h"
#include "webservice.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // No se muy bien que hace esto, pero es para que no salte:
    // strcmp(locale, "C"):Error:Assert failed:in file /home/cosimani/Proyecto/DatosSeguros/tesseract-ocr/src/api/baseapi.cpp
    // Al querer instanciar un objeto de tesseract::TessBaseAPI
    setlocale( LC_ALL, "C" );

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

    Logger::getInstance()->setPrefixArchivoLog( Config::getInstance()->getString( "log_dir" ) + "DatosSeguros_" );

    WebService * webService = new WebService;
    webService->iniciar( Config::getInstance()->getString( "puerto_tcp" ).toInt() );

    return a.exec();
}



