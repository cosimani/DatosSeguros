#include "webservice.h"
#include <QDebug>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDateTime>
#include <QThread>

#include "procesador.h"
#include "validador.h"

WebService::WebService( QObject * parent ) : QObject( parent ),
                                             tcpServer( new QTcpServer( this ) ),
                                             decoder( new QZXing )
{

    decoder->setDecoder( QZXing::DecoderFormat_PDF_417 );
}

WebService::~WebService()
{
    tcpServer->close();
    qDeleteAll( listTcpSockets.begin(), listTcpSockets.end() );
}

bool WebService::iniciar( int puerto )
{
    bool exito = tcpServer->listen( QHostAddress::Any, puerto );
    if ( exito )  {
        qDebug() << "WebService escuchando en puerto" << puerto;

        connect( tcpServer, SIGNAL( newConnection() ), this, SLOT( onNewConnection() ) );

        return true;
    }
    else  {
        qDebug() << "EL WebService NOOOO se pudo levantar en puerto" << puerto;
    }

    return false;
}

bool WebService::isEscuchando()  {
    return tcpServer->isListening();
}

void WebService::onNewConnection()  {
    QTcpSocket * tcpSocket = tcpServer->nextPendingConnection();

    connect( tcpSocket, SIGNAL( readyRead() ), this, SLOT( slot_leerDatos() ) );
    connect( tcpSocket, SIGNAL( disconnected() ), this, SLOT( socketDisconnected() ) );

    listTcpSockets << tcpSocket;

    Conexion * conexion = new Conexion;
    conexion->setTcpSocket( tcpSocket );

    // Se agrega una Conexion para cada una de las consultas desde los clientes. En este momento no se dispone de
    // la informacion que viene en la solicitud, pero no importa, despues se agrega la info del tipo de carnet que
    // se desea analizar. Esta Conexion sera eliminada del vector luego que este socket se finalice.
    vConexiones.append( conexion );

    qDebug() << "Nueva conexion. tcpSocket = " << tcpSocket;

    qDebug() << "Conexiones totales = " << vConexiones.size();
    for( int i = 0 ; i < vConexiones.size() ; i++ )  {
        Conexion * conexion = vConexiones.at( i );
        qDebug() << conexion->getTcpSocket();
    }



}


/**
 * @brief WebService::slot_leerDatos
 * - Este metodo atiende solo las peticiones a /index.php
 * - No tiene en cuenta nada que se envie en la cabecera HTTP
 * - Solo peticiones GET
 * - Igualmente emite la senal signal_solicitudRecibida con todos los datos recibidos
 */
void WebService::slot_leerDatos()  {
    QTcpSocket * tcpSocket = qobject_cast< QTcpSocket * >( sender() );

    // Utiliza el tcpSocket que corresponde
    for( int i = 0 ; i < vConexiones.size() ; i++ )  {

        Conexion * conexion = vConexiones.at( i );

        if ( conexion->getTcpSocket() == tcpSocket )  {

            QByteArray ba = tcpSocket->readAll();
            QStringList tokens = QString( ba ).split( QRegExp( "\\s+" ) );  // Separa con espacios y lineas nuevas

            // Las siguientes son validaciones varias para que no se vaya a clavar este programa
            if ( tokens.size() == 0 )  {
                return;
            }

            ///////////////////////////////
            /// Un par de validaciones
            ///////////////////////////////

            if ( tokens.at( 0 ) == "GET" || tokens.at( 0 ) == "HEAD" )  {
                tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                tcpSocket->write( "\r\n" );
                tcpSocket->write( "{ \"error\":\"El server atiende sólo solucitudes POST al recibir.php.\" }" );
                tcpSocket->flush();
                tcpSocket->close();
            }

            ///////////////////////////////
            /// FIN - Un par de validaciones
            ///////////////////////////////




            ////////////////////////////////////////////////////////
            /// PASO 1
            /// Analiza si es POST, extrae Content-Length, extrae carnet, extrae usuario, completa estos datos en
            /// la Conexion correspondiente dentro de vConexiones.
            ////////////////////////////////////////////////////////
            if ( tokens.at( 0 ) == "POST" )  {

                // Este es el index del string donde esta Content-Length
                int indexContentLength = tokens.indexOf( "Content-Length:" );

                // Este for actualiza el sizeDeLaImagen de la Conexion que corresponde a este tcpSocket
                conexion->setSizeDeLaImagen( indexContentLength );

                // Si Content-Length existe, entonces el string siguiente es el numero de bytes que tiene la imagen
                if ( tokens.size() > indexContentLength )  {
                    QString strBytesDeLaImagen = tokens.at( indexContentLength + 1 );
                    conexion->setBytesRestantes( strBytesDeLaImagen.toInt() );
//                    bytesRestantesDeLaImagenPorLeer = strBytesDeLaImagen.toInt();
                }

                QString token2 = tokens.at( 1 );  // token2 es algo como /recibir.php?usuario=cosimani&carnet=licencia

                QString recibir_php = "recibir.php";

                // Entra a este if cuando se recibion una consulta correcta con recibir.php
                if ( token2.contains( recibir_php, Qt::CaseInsensitive ) )  {

                    // Si entro a este if, entonces tokens es algo como lo siguiente:
                    //      /recibir.php?q=6&aver=9
                    //      /recibir.php?consulta=usuarios&key=1234

                    QStringList recurso_y_variables = token2.split( "?" );

                    // Entra si la consulta tiene una o mas variables
                    if ( recurso_y_variables.size() >= 2 )  {

                        // query contendra todas las variables para se interpretada con metodos de QUrlQuery
                        QUrlQuery query( recurso_y_variables.at( 1 ) );

                        // Se actualiza el tipo de carnet que solicita analizar este tcpSocket
                        QString tipoCarnet = query.queryItemValue( "carnet" );
                        QString usuario = query.queryItemValue( "usuario" );
                        conexion->setTipo( tipoCarnet );
                        conexion->setUsuario( usuario );
                    }
                }
            ////////////////////////////////////////////////////////
            /// FIN - PASO 1
            ////////////////////////////////////////////////////////
            }
            else  { // Si entra aca es porque ya solo vienen datos binarios para armar la imagen

                ////////////////////////////////////////////////////////
                /// PASO 2
                /// Se van obteniendo los datos binarios para formar la imagen
                ////////////////////////////////////////////////////////

                conexion->appendData( ba );

                int bytesRestantes = conexion->getBytesRestantes() - ba.count();
                conexion->setBytesRestantes( bytesRestantes );

                ////////////////////////////////////////////////////////
                /// FIN - PASO 2
                /// Solo las tres lineas anteriores se ejecutan para formar la imagen
                ////////////////////////////////////////////////////////


                ////////////////////////////////////////////////////////
                /// PASO 3
                /// Cuando los bytes restantes es cero, entonces se procede a analizar la imagen
                ////////////////////////////////////////////////////////
                if ( conexion->getBytesRestantes() <= 0 )  {

                    if ( im.loadFromData( conexion->getImData(), "JPG" ) )  {
                        im = im.convertToFormat( QImage::Format_RGB888 );

                        QString fecha_hora = QDateTime::currentDateTime().toString( "yyyyMMddhhmmss" );

        #ifdef EJECUTADO_EN_SERVER

                        conexion->setFecha_hora_foto( fecha_hora );
                        QString nombreArchivo = "imagenes/registros/" + conexion->getFecha_hora_foto() + ".jpg";
        #else
                        QString nombreArchivo = "../" + fecha_hora + ".jpg";
        #endif
                        // Guardamos la imagen que el usuario envio para analizar
                        im.save( nombreArchivo.toStdString().c_str() );

                        conexion->clearData();
                    }
                    else  {

                        tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                        tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                        tcpSocket->write( "\r\n" );
                        tcpSocket->write( "{ \"error\":\"El server no pudo recibir la imagen correctamente.\" }" );
                        tcpSocket->flush();
                        tcpSocket->close();

                        // este return es porque no se pudo crear la imagen con los datos que envio el usuario
                        return;
                    }


                    cv::Mat imParaAlinear( im.height(), im.width(),
                                           CV_8UC3, ( void * )im.constBits(), im.bytesPerLine() );

                    cv::cvtColor( imParaAlinear, imParaAlinear, cv::COLOR_RGB2BGR );

#ifdef EJECUTADO_EN_SERVER
                    QString paraAlinear = "imagenes/registros/" + conexion->getFecha_hora_foto() + "_paraAlinear.jpg";
                    cv::imwrite( paraAlinear.toStdString().c_str(), imParaAlinear );
#else
                    cv::imwrite( "../imagenes/registros/paraAlinear.jpg", imParaAlinear );
#endif

                    if ( conexion->getTipo() == "dni" )  {
                        qDebug() << "dni";

                        QString resultadoZXing;
                        cv::Mat matParaLeerCodigo;

                        cv::cvtColor( imParaAlinear, matParaLeerCodigo, CV_RGB2GRAY );

            //            cv::threshold( matParaLeerCodigo, matParaLeerCodigo, umbral, 255, cv::THRESH_BINARY );

            //            //Erode the image with 3x3 kernel
            //            cv::erode( matParaLeerCodigo, matParaLeerCodigo,
            //                       cv::getStructuringElement( cv::MORPH_RECT, cv::Size( 3, 3 ) ) );

                        cv::adaptiveThreshold( matParaLeerCodigo, matParaLeerCodigo, 255,
                                               CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY, 91, 15 );

    #ifdef EJECUTADO_EN_SERVER
                        QString paraLeerCodigo1 = "imagenes/registros/" +
                                                  conexion->getFecha_hora_foto() + "_paraLeerCodigo1.jpg";
                        cv::imwrite( paraLeerCodigo1.toStdString().c_str(), matParaLeerCodigo );
    #else
                        cv::imwrite( "../imagenes/registros/matParaLeerCodigo1.jpg", matParaLeerCodigo );
    #endif


                        cv::Mat imAlineada;

                        // Busca el codigo antes de alinear
                        resultadoZXing = decoder->decodeImage( QImage( matParaLeerCodigo.data, matParaLeerCodigo.cols,
                                                                       matParaLeerCodigo.rows, matParaLeerCodigo.step,
                                                                       QImage::Format_Grayscale8 ) );

                        Procesador::getInstancia()->alinear( imParaAlinear, imAlineada, Procesador::DNI );

                        if ( imAlineada.rows == 0 )  {
                            tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                            tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                            tcpSocket->write( "\r\n" );
                            tcpSocket->write( "{ \"error\":\"Parece que no hay un DNI en la imagen.\" }" );
                            tcpSocket->flush();
                            tcpSocket->close();

                            return;
                        }

                        if ( resultadoZXing.isEmpty() )  {
                            cv::cvtColor( imAlineada, matParaLeerCodigo, CV_RGB2GRAY );
            //                cv::threshold( matParaLeerCodigo, matParaLeerCodigo, umbral, 255, cv::THRESH_BINARY );

                            cv::adaptiveThreshold( matParaLeerCodigo, matParaLeerCodigo, 255,
                                                   CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY, 91, 15 );

#ifdef EJECUTADO_EN_SERVER
                            QString paraLeerCodigo2 = "imagenes/registros/" +
                                                      conexion->getFecha_hora_foto() + "_paraLeerCodigo2.jpg";
                            cv::imwrite( paraLeerCodigo2.toStdString().c_str(), matParaLeerCodigo );
#else
                            cv::imwrite( "../imagenes/registros/matParaLeerCodigo2.jpg", matParaLeerCodigo );
#endif
                            resultadoZXing = decoder->decodeImage( QImage( matParaLeerCodigo.data,
                                                                           matParaLeerCodigo.cols,
                                                                           matParaLeerCodigo.rows,
                                                                           matParaLeerCodigo.step,
                                                                           QImage::Format_Grayscale8 ) );
                        }

#ifdef EJECUTADO_EN_SERVER
                        QString alineada = "imagenes/registros/" + conexion->getFecha_hora_foto() + "_alineada.jpg";
                        cv::imwrite( alineada.toStdString().c_str(), imAlineada );
#else
                        cv::imwrite( "../imagenes/registros/alineada.jpg", imAlineada );
#endif

                        cv::Mat imConRectangulos;

                        QStringList datosExtraidos = Procesador::getInstancia()->extraerInfo( imAlineada,
                                                                                              imConRectangulos,
                                                                                              Procesador::DNI );

#ifdef EJECUTADO_EN_SERVER
                        QString conRectangulos = "imagenes/registros/" +
                                                 conexion->getFecha_hora_foto() + "_conRectangulos.jpg";
                        cv::imwrite( conRectangulos.toStdString().c_str(), imConRectangulos );

#else
                        cv::imwrite( "../imagenes/registros/conRectangulos.jpg", imConRectangulos );
#endif

                        cv::Mat imFotoPerfil;

                        Procesador::getInstancia()->extraerFoto( imAlineada, imFotoPerfil, Procesador::DNI );

#ifdef EJECUTADO_EN_SERVER
                        QString fotoPerfil = "imagenes/registros/" + conexion->getFecha_hora_foto() + "_fotoPerfil.jpg";
                        cv::imwrite( fotoPerfil.toStdString().c_str(), imFotoPerfil );
#else
                        cv::imwrite( "../imagenes/registros/fotoPerfil.jpg", imFotoPerfil );
#endif

                        QString json;

                        // Este if es para cuando no se pudo con el codigo, entocnes devuelve los datos de la imagen
                        if ( resultadoZXing.isEmpty() )  {
                            json = Validador::getInstance()->validarDniDeImagen( datosExtraidos );
                        }
                        else  {
                            json = Validador::getInstance()->validaDniDeCodigo( resultadoZXing.split( "@" ) );
                        }

                        qDebug() << json;

                        if ( json.isEmpty() )  {  // Si el json esta vacio es porque no fueron validos los datos
                            tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                            tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                            tcpSocket->write( "\r\n" );
                            tcpSocket->write( "{ \"error\":\"No se pudieron extraer correctamente los datos.\" }" );
                            tcpSocket->flush();
                            tcpSocket->close();
                        }
                        else  {
                            tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                            tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                            tcpSocket->write( "\r\n" );
                            tcpSocket->write( json.toStdString().c_str() );
                            tcpSocket->flush();
                            tcpSocket->close();
                        }
                    }
                    else if ( conexion->getTipo() == "dni-dorso" )  {
                        qDebug() << "dni-dorso";

                        cv::Mat matParaLeerCodigo;

                        cv::cvtColor( imParaAlinear, matParaLeerCodigo, CV_RGB2GRAY );

            //            cv::threshold( matParaLeerCodigo, matParaLeerCodigo, umbral, 255, cv::THRESH_BINARY );

            //            //Erode the image with 3x3 kernel
            //            cv::erode( matParaLeerCodigo, matParaLeerCodigo,
            //                       cv::getStructuringElement( cv::MORPH_RECT, cv::Size( 3, 3 ) ) );

                        cv::adaptiveThreshold( matParaLeerCodigo, matParaLeerCodigo, 255,
                                               CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY, 91, 15 );

    #ifdef EJECUTADO_EN_SERVER
                        QString paraLeerCodigo1 = "imagenes/registros/" +
                                                  conexion->getFecha_hora_foto() + "_paraLeerCodigo1.jpg";
                        cv::imwrite( paraLeerCodigo1.toStdString().c_str(), matParaLeerCodigo );
    #else
                        cv::imwrite( "../imagenes/registros/matParaLeerCodigo1.jpg", matParaLeerCodigo );
    #endif


                        cv::Mat imAlineada;

                        Procesador::getInstancia()->alinear( imParaAlinear, imAlineada, Procesador::DNI_DORSO );

                        if ( imAlineada.rows == 0 )  {
                            tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                            tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                            tcpSocket->write( "\r\n" );
                            tcpSocket->write( "{ \"error\":\"Parece que no hay un DNI de dorso en la imagen.\" }" );
                            tcpSocket->flush();
                            tcpSocket->close();

                            return;
                        }


#ifdef EJECUTADO_EN_SERVER
                        QString alineada = "imagenes/registros/" + conexion->getFecha_hora_foto() + "_alineada.jpg";
                        cv::imwrite( alineada.toStdString().c_str(), imAlineada );
#else
                        cv::imwrite( "../imagenes/registros/alineada.jpg", imAlineada );
#endif

                        cv::Mat imConRectangulos;

                        QStringList datosExtraidos = Procesador::getInstancia()->extraerInfo( imAlineada,
                                                                                              imConRectangulos,
                                                                                              Procesador::DNI_DORSO );

#ifdef EJECUTADO_EN_SERVER
                        QString conRectangulos = "imagenes/registros/" +
                                                 conexion->getFecha_hora_foto() + "_conRectangulos.jpg";
                        cv::imwrite( conRectangulos.toStdString().c_str(), imConRectangulos );

#else
                        cv::imwrite( "../imagenes/registros/conRectangulos.jpg", imConRectangulos );
#endif
                        QString json = Validador::getInstance()->validaDniDorso( datosExtraidos );

                        qDebug() << json;

                        if ( json.isEmpty() )  {  // Si el json esta vacio es porque no fueron validos los datos
                            tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                            tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                            tcpSocket->write( "\r\n" );
                            tcpSocket->write( "{ \"error\":\"No se pudieron extraer correctamente los datos.\" }" );
                            tcpSocket->flush();
                            tcpSocket->close();
                        }
                        else  {
                            tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                            tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                            tcpSocket->write( "\r\n" );
                            tcpSocket->write( json.toStdString().c_str() );
                            tcpSocket->flush();
                            tcpSocket->close();
                        }
                    }
                    else if ( conexion->getTipo() == "licencia" )  {
                        qDebug() << "licencia";

                        cv::Mat matParaLeerCodigo;

                        cv::cvtColor( imParaAlinear, matParaLeerCodigo, CV_RGB2GRAY );

            //            cv::threshold( matParaLeerCodigo, matParaLeerCodigo, umbral, 255, cv::THRESH_BINARY );

            //            //Erode the image with 3x3 kernel
            //            cv::erode( matParaLeerCodigo, matParaLeerCodigo,
            //                       cv::getStructuringElement( cv::MORPH_RECT, cv::Size( 3, 3 ) ) );

                        cv::adaptiveThreshold( matParaLeerCodigo, matParaLeerCodigo, 255,
                                               CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY, 91, 15 );

    #ifdef EJECUTADO_EN_SERVER
                        QString paraLeerCodigo1 = "imagenes/registros/" +
                                                  conexion->getFecha_hora_foto() + "_paraLeerCodigo1.jpg";
                        cv::imwrite( paraLeerCodigo1.toStdString().c_str(), matParaLeerCodigo );
    #else
                        cv::imwrite( "../imagenes/registros/matParaLeerCodigo1.jpg", matParaLeerCodigo );
    #endif


                        cv::Mat imAlineada;

                        Procesador::getInstancia()->alinear( imParaAlinear, imAlineada, Procesador::LICENCIA );

                        if ( imAlineada.rows == 0 )  {
                            tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                            tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                            tcpSocket->write( "\r\n" );
                            tcpSocket->write( "{ \"error\":\"Parece que no hay una licencia de Cba en la imagen.\" }" );
                            tcpSocket->flush();
                            tcpSocket->close();

                            return;
                        }

#ifdef EJECUTADO_EN_SERVER
                        QString alineada = "imagenes/registros/" + conexion->getFecha_hora_foto() + "_alineada.jpg";
                        cv::imwrite( alineada.toStdString().c_str(), imAlineada );

#else
                        cv::imwrite( "../imagenes/registros/alineada.jpg", imAlineada );
#endif


                        cv::Mat imConRectangulos;

                        QStringList datosExtraidos = Procesador::getInstancia()->extraerInfo( imAlineada,
                                                                                              imConRectangulos,
                                                                                              Procesador::LICENCIA );

#ifdef EJECUTADO_EN_SERVER
                        QString conRectangulos = "imagenes/registros/" +
                                                 conexion->getFecha_hora_foto() + "_conRectangulos.jpg";
                        cv::imwrite( conRectangulos.toStdString().c_str(), imConRectangulos );

#else
                        cv::imwrite( "../imagenes/registros/conRectangulos.jpg", imConRectangulos );
#endif


                        cv::Mat imFotoPerfil;

                        Procesador::getInstancia()->extraerFoto( imAlineada, imFotoPerfil, Procesador::LICENCIA );

#ifdef EJECUTADO_EN_SERVER
                        QString fotoPerfil = "imagenes/registros/" + conexion->getFecha_hora_foto() + "_fotoPerfil.jpg";
                        cv::imwrite( fotoPerfil.toStdString().c_str(), imFotoPerfil );
#else
                        cv::imwrite( "../imagenes/registros/fotoPerfil.jpg", imFotoPerfil );
#endif

                        QString json = Validador::getInstance()->validarLicenciaDeImagen( datosExtraidos );

                        qDebug() << json;

                        tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                        tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                        tcpSocket->write( "\r\n" );
                        tcpSocket->write( json.toStdString().c_str() );
                        tcpSocket->flush();
                        tcpSocket->close();
                    }
                    else if ( conexion->getTipo() == "licencia-dorso" )  {
                        qDebug() << "licencia-dorso";

                        cv::Mat matParaLeerCodigo;

                        cv::cvtColor( imParaAlinear, matParaLeerCodigo, CV_RGB2GRAY );

            //            cv::threshold( matParaLeerCodigo, matParaLeerCodigo, umbral, 255, cv::THRESH_BINARY );

            //            //Erode the image with 3x3 kernel
            //            cv::erode( matParaLeerCodigo, matParaLeerCodigo,
            //                       cv::getStructuringElement( cv::MORPH_RECT, cv::Size( 3, 3 ) ) );

                        cv::adaptiveThreshold( matParaLeerCodigo, matParaLeerCodigo, 255,
                                               CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY, 91, 15 );

    #ifdef EJECUTADO_EN_SERVER
                        QString paraLeerCodigo1 = "imagenes/registros/" +
                                                  conexion->getFecha_hora_foto() + "_paraLeerCodigo1.jpg";
                        cv::imwrite( paraLeerCodigo1.toStdString().c_str(), matParaLeerCodigo );
    #else
                        cv::imwrite( "../imagenes/registros/matParaLeerCodigo1.jpg", matParaLeerCodigo );
    #endif


                        cv::Mat imAlineada;

                        Procesador::getInstancia()->alinear( imParaAlinear, imAlineada, Procesador::LICENCIA_DORSO );

                        if ( imAlineada.rows == 0 )  {
                            tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                            tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                            tcpSocket->write( "\r\n" );
                            tcpSocket->write( "{ \"error\":\"Parece que no hay una licencia de dorso en la imagen.\" }" );
                            tcpSocket->flush();
                            tcpSocket->close();

                            return;
                        }


#ifdef EJECUTADO_EN_SERVER
                        QString alineada = "imagenes/registros/" + conexion->getFecha_hora_foto() + "_alineada.jpg";
                        cv::imwrite( alineada.toStdString().c_str(), imAlineada );
#else
                        cv::imwrite( "../imagenes/registros/alineada.jpg", imAlineada );
#endif

                        cv::Mat imConRectangulos;

                        QStringList datosExtraidos = Procesador::getInstancia()->extraerInfo( imAlineada,
                                                                                              imConRectangulos,
                                                                                              Procesador::LICENCIA_DORSO );

#ifdef EJECUTADO_EN_SERVER
                        QString conRectangulos = "imagenes/registros/" +
                                                 conexion->getFecha_hora_foto() + "_conRectangulos.jpg";
                        cv::imwrite( conRectangulos.toStdString().c_str(), imConRectangulos );

#else
                        cv::imwrite( "../imagenes/registros/conRectangulos.jpg", imConRectangulos );
#endif
                        QString json = Validador::getInstance()->validaLicenciaDorso( datosExtraidos );

                        qDebug() << json;

                        if ( json.isEmpty() )  {  // Si el json esta vacio es porque no fueron validos los datos
                            tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                            tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                            tcpSocket->write( "\r\n" );
                            tcpSocket->write( "{ \"error\":\"No se pudieron extraer correctamente los datos.\" }" );
                            tcpSocket->flush();
                            tcpSocket->close();
                        }
                        else  {
                            tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                            tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                            tcpSocket->write( "\r\n" );
                            tcpSocket->write( json.toStdString().c_str() );
                            tcpSocket->flush();
                            tcpSocket->close();
                        }
                    }
                    else if ( conexion->getTipo() == "verde" )  {
                        qDebug() << "verde";

                        cv::Mat matParaLeerCodigo;

                        cv::cvtColor( imParaAlinear, matParaLeerCodigo, CV_RGB2GRAY );

            //            cv::threshold( matParaLeerCodigo, matParaLeerCodigo, umbral, 255, cv::THRESH_BINARY );

            //            //Erode the image with 3x3 kernel
            //            cv::erode( matParaLeerCodigo, matParaLeerCodigo,
            //                       cv::getStructuringElement( cv::MORPH_RECT, cv::Size( 3, 3 ) ) );

                        cv::adaptiveThreshold( matParaLeerCodigo, matParaLeerCodigo, 255,
                                               CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY, 91, 15 );

    #ifdef EJECUTADO_EN_SERVER
                        QString paraLeerCodigo1 = "imagenes/registros/" +
                                                  conexion->getFecha_hora_foto() + "_paraLeerCodigo1.jpg";
                        cv::imwrite( paraLeerCodigo1.toStdString().c_str(), matParaLeerCodigo );
    #else
                        cv::imwrite( "../imagenes/registros/matParaLeerCodigo1.jpg", matParaLeerCodigo );
    #endif


                        cv::Mat imAlineada;

                        Procesador::getInstancia()->alinear( imParaAlinear, imAlineada, Procesador::VERDE );

                        if ( imAlineada.rows == 0 )  {
                            tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                            tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                            tcpSocket->write( "\r\n" );
                            tcpSocket->write( "{ \"error\":\"Parece que no hay una tarjeta verde de frente en la imagen.\" }" );
                            tcpSocket->flush();
                            tcpSocket->close();

                            return;
                        }


#ifdef EJECUTADO_EN_SERVER
                        QString alineada = "imagenes/registros/" + conexion->getFecha_hora_foto() + "_alineada.jpg";
                        cv::imwrite( alineada.toStdString().c_str(), imAlineada );
#else
                        cv::imwrite( "../imagenes/registros/alineada.jpg", imAlineada );
#endif

                        cv::Mat imConRectangulos;

                        QStringList datosExtraidos = Procesador::getInstancia()->extraerInfo( imAlineada,
                                                                                              imConRectangulos,
                                                                                              Procesador::VERDE );

#ifdef EJECUTADO_EN_SERVER
                        QString conRectangulos = "imagenes/registros/" +
                                                 conexion->getFecha_hora_foto() + "_conRectangulos.jpg";
                        cv::imwrite( conRectangulos.toStdString().c_str(), imConRectangulos );

#else
                        cv::imwrite( "../imagenes/registros/conRectangulos.jpg", imConRectangulos );
#endif
                        QString json = Validador::getInstance()->validaVerde( datosExtraidos );

                        qDebug() << json;

                        if ( json.isEmpty() )  {  // Si el json esta vacio es porque no fueron validos los datos
                            tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                            tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                            tcpSocket->write( "\r\n" );
                            tcpSocket->write( "{ \"error\":\"No se pudieron extraer correctamente los datos.\" }" );
                            tcpSocket->flush();
                            tcpSocket->close();
                        }
                        else  {
                            tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                            tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                            tcpSocket->write( "\r\n" );
                            tcpSocket->write( json.toStdString().c_str() );
                            tcpSocket->flush();
                            tcpSocket->close();
                        }
                    }
                    else if ( conexion->getTipo() == "verde-dorso" )  {
                        qDebug() << "verde-dorso";

                        cv::Mat matParaLeerCodigo;

                        cv::cvtColor( imParaAlinear, matParaLeerCodigo, CV_RGB2GRAY );

            //            cv::threshold( matParaLeerCodigo, matParaLeerCodigo, umbral, 255, cv::THRESH_BINARY );

            //            //Erode the image with 3x3 kernel
            //            cv::erode( matParaLeerCodigo, matParaLeerCodigo,
            //                       cv::getStructuringElement( cv::MORPH_RECT, cv::Size( 3, 3 ) ) );

                        cv::adaptiveThreshold( matParaLeerCodigo, matParaLeerCodigo, 255,
                                               CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY, 91, 15 );

    #ifdef EJECUTADO_EN_SERVER
                        QString paraLeerCodigo1 = "imagenes/registros/" +
                                                  conexion->getFecha_hora_foto() + "_paraLeerCodigo1.jpg";
                        cv::imwrite( paraLeerCodigo1.toStdString().c_str(), matParaLeerCodigo );
    #else
                        cv::imwrite( "../imagenes/registros/matParaLeerCodigo1.jpg", matParaLeerCodigo );
    #endif


                        cv::Mat imAlineada;

                        Procesador::getInstancia()->alinear( imParaAlinear, imAlineada, Procesador::VERDE_DORSO );

                        if ( imAlineada.rows == 0 )  {
                            tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                            tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                            tcpSocket->write( "\r\n" );
                            tcpSocket->write( "{ \"error\":\"Parece que no hay una tarjeta verde de dorso en la imagen.\" }" );
                            tcpSocket->flush();
                            tcpSocket->close();

                            return;
                        }


#ifdef EJECUTADO_EN_SERVER
                        QString alineada = "imagenes/registros/" + conexion->getFecha_hora_foto() + "_alineada.jpg";
                        cv::imwrite( alineada.toStdString().c_str(), imAlineada );
#else
                        cv::imwrite( "../imagenes/registros/alineada.jpg", imAlineada );
#endif

                        cv::Mat imConRectangulos;

                        QStringList datosExtraidos = Procesador::getInstancia()->extraerInfo( imAlineada,
                                                                                              imConRectangulos,
                                                                                              Procesador::VERDE_DORSO );

#ifdef EJECUTADO_EN_SERVER
                        QString conRectangulos = "imagenes/registros/" +
                                                 conexion->getFecha_hora_foto() + "_conRectangulos.jpg";
                        cv::imwrite( conRectangulos.toStdString().c_str(), imConRectangulos );

#else
                        cv::imwrite( "../imagenes/registros/conRectangulos.jpg", imConRectangulos );
#endif
                        QString json = Validador::getInstance()->validaVerdeDorso( datosExtraidos );

                        qDebug() << json;

                        if ( json.isEmpty() )  {  // Si el json esta vacio es porque no fueron validos los datos
                            tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                            tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                            tcpSocket->write( "\r\n" );
                            tcpSocket->write( "{ \"error\":\"No se pudieron extraer correctamente los datos.\" }" );
                            tcpSocket->flush();
                            tcpSocket->close();
                        }
                        else  {
                            tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                            tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                            tcpSocket->write( "\r\n" );
                            tcpSocket->write( json.toStdString().c_str() );
                            tcpSocket->flush();
                            tcpSocket->close();
                        }
                    }
                    else  {
                        qDebug() << "ninguno";
                    }


                ////////////////////////////////////////////////////////
                /// FIN - PASO 3
                /// Dentro de este paso 3 se analiza la imagen recibida
                ////////////////////////////////////////////////////////
                }
            }
        }
    }
}

void WebService::socketDisconnected()
{
    QTcpSocket * tcpSocket = qobject_cast< QTcpSocket * >( sender() );
    qDebug() << "socketDisconnected:" << tcpSocket;

    // Este slot se llama cuando un socket es finalizado. Entonces lo eliminamos del vector de Conexion
    if ( tcpSocket )  {
        for( int i = 0 ; i < vConexiones.size() ; i++ )  {
            Conexion * conexion = vConexiones.at( i );

            if ( conexion->getTcpSocket() == tcpSocket )  {
                vConexiones.removeAt( i );
            }
        }
    }

    if ( tcpSocket ) {
//        listTcpSockets.removeAll( tcpSocket );
        listTcpSockets.removeOne( tcpSocket );

        disconnect( tcpSocket, SIGNAL( readyRead() ), this, SLOT( slot_leerDatos() ) );
        disconnect( tcpSocket, SIGNAL( disconnected() ), this, SLOT( socketDisconnected() ) );

        tcpSocket->deleteLater();
    }
}



