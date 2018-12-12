#include <QCoreApplication>

#include "procesador.h"
#include "config.h"
#include "logger.h"
#include "webservice.h"
#include "validador.h"

#include <QDir>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // No se muy bien que hace esto, pero es para que no salte:
    // strcmp(locale, "C"):Error:Assert failed:in file /home/cosimani/Proyecto/DatosSeguros/tesseract-ocr/src/api/baseapi.cpp
    // Al querer instanciar un objeto de tesseract::TessBaseAPI
    setlocale( LC_ALL, "C" );

#ifdef EJECUTADO_EN_SERVER

    Procesador::getInstancia()->configurarImageAlignment( Procesador::DNI,
                                                          "imagenes/referencias/DniFrente.jpg" );

    Procesador::getInstancia()->configurarImageAlignment( Procesador::LICENCIA,
                                                          "imagenes/referencias/LicenciaFrente.jpg" );

    Procesador::getInstancia()->configurarImageAlignment( Procesador::VERDE,
                                                          "imagenes/referencias/VerdeFrente.jpg" );

    Procesador::getInstancia()->configurarImageAlignment( Procesador::DNI_DORSO,
                                                          "imagenes/referencias/DniDorso.jpg" );

    Procesador::getInstancia()->configurarImageAlignment( Procesador::LICENCIA_DORSO,
                                                          "imagenes/referencias/LicenciaDorso.jpg" );

    Procesador::getInstancia()->configurarImageAlignment( Procesador::VERDE_DORSO,
                                                          "imagenes/referencias/VerdeDorso.jpg" );



#else

    Procesador::getInstancia()->configurarImageAlignment( Procesador::LICENCIA,
                                                          "../imagenes/referencias/LicenciaFrente.jpg" );

    Procesador::getInstancia()->configurarImageAlignment( Procesador::DNI,
                                                          "../imagenes/referencias/DniFrente.jpg" );

//    QFile file( "../imagenes/referencias/DniFrente.jpg" );
//    qDebug() << file.exists();

#endif

    qDebug() << endl << "Directorio de trabajo: " + QDir::currentPath();

    if ( ! Config::getInstance()->isOk() )  {
        qDebug() << endl << "No se encuentra el archivo de configuracion. Cerrar la app.";

    }
    else  {

        // Cuando setPrefixArchivoLog no es invocado, entonces out tiene valor CONSOLE
        Logger::getInstance()->setPrefixArchivoLog( Config::getInstance()->getString( "log_dir" ) + "DatosSeguros_" );

        Validador::getInstance();

        WebService * webService = new WebService;
        webService->iniciar( Config::getInstance()->getString( "puerto_tcp" ).toInt() );

    }

    return a.exec();
}



