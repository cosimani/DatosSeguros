#include <QCoreApplication>

#include "procesador.h"
#include "config.h"
#include "logger.h"
#include "webservice.h"
#include "validador.h"
#include "testeador.h"

#include <QDir>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // No se muy bien que hace esto, pero es para que no salte:
    // strcmp(locale, "C"):Error:Assert failed:in file /home/cosimani/Proyecto/DatosSeguros/tesseract-ocr/src/api/baseapi.cpp
    // Al querer instanciar un objeto de tesseract::TessBaseAPI
    setlocale( LC_ALL, "C" );

    WebService * webService = new WebService;
    QObject::connect( webService, SIGNAL( signal_cerrarAplicacion() ), &a, SLOT( quit() ) );


    Procesador::getInstancia()->configurarImageAlignment( Procesador::DNI,
                                                          "imagenes/referencias/DniFrente.jpg", 500, 0.10f );

    Procesador::getInstancia()->configurarImageAlignment( Procesador::LICENCIA,
                                                          "imagenes/referencias/LicenciaFrente.jpg", 500, 0.15f );

    Procesador::getInstancia()->configurarImageAlignment( Procesador::VERDE,
                                                          "imagenes/referencias/VerdeFrente.jpg", 500, 0.15f );

    Procesador::getInstancia()->configurarImageAlignment( Procesador::DNI_DORSO,
                                                          "imagenes/referencias/DniDorso.jpg", 500, 0.15f );

    Procesador::getInstancia()->configurarImageAlignment( Procesador::LICENCIA_DORSO,
                                                          "imagenes/referencias/LicenciaDorso.jpg", 500, 0.15f );

    Procesador::getInstancia()->configurarImageAlignment( Procesador::VERDE_DORSO,
                                                          "imagenes/referencias/VerdeDorso.jpg", 500, 0.15f );


//    qDebug() << endl << "Directorio de trabajo: " + QDir::currentPath();

    if ( ! Config::getInstance()->isOk() )  {
        qDebug() << endl << "No se encuentra el archivo de configuracion. Cerrar la app.";

    }
    else  {

        // Cuando setPrefixArchivoLog no es invocado, entonces out tiene valor CONSOLE
        Logger::getInstance()->setPrefixArchivoLog( Config::getInstance()->getString( "log_dir" ) + "DatosSeguros_" );

        Validador::getInstance();

        // Iniciamos y conectamos la DB
        Database::getInstance()->connect();

        webService->iniciar( Config::getInstance()->getString( "puerto_tcp" ).toInt() );

//        Testeador testeador;
//        testeador.procesarDniDorso( "/home/cosimani/Proyecto/DatosSeguros/GitHub/testing/dni-dorso-sin-recortar" );


    }

    return a.exec();
}



