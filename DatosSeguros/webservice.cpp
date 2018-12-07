#include "webservice.h"
#include <QDebug>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDateTime>
#include <QThread>

#include "procesador.h"

WebService::WebService( QObject * parent ) : QObject( parent ),
                                             tcpServer( new QTcpServer( this ) ),
                                             bytesRestantesDeLaImagenPorLeer( 0 ),
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


    // Averigua que tipo de carnet solicita analizar este tcpSocket
    for( int i = 0 ; i < vConexiones.size() ; i++ )  {
        Conexion * conexion = vConexiones.at( i );
        if ( conexion->getTcpSocket() == tcpSocket )  {

            QByteArray ba = tcpSocket->readAll();
            QStringList tokens = QString( ba ).split( QRegExp( "\\s+" ) );  // Separa con espacios y lineas nuevas

            // Las siguientes son validaciones varias para que no se vaya a clavar este programa
            if ( tokens.size() == 0 )  {
                return;
            }

            ////////////////////////////////////////////////////////
            /// PASO 1
            /// Analiza si es POST, extrae Content-Length, extrae carnet, extrae usuario, completa estos datos en
            /// la Conexion correspondiente dentro de vConexiones.
            ////////////////////////////////////////////////////////
            if ( tokens.at( 0 ) == "POST" )  {

        //        qDebug() << tokens;

                // Este es el index del string donde esta Content-Length
                int indexContentLength = tokens.indexOf( "Content-Length:" );

                // Este for actualiza el sizeDeLaImagen de la Conexion que corresponde a este tcpSocket
                for( int i = 0 ; i < vConexiones.size() ; i++ )  {
                    Conexion * conexion = vConexiones.at( i );
                    if ( conexion->getTcpSocket() == tcpSocket )  {
                        conexion->setSizeDeLaImagen( indexContentLength );
                    }
                }

                // Si Content-Length existe, entonces el string siguiente es el numero de bytes que tiene la imagen
                if ( tokens.size() > indexContentLength )  {
                    QString strBytesDeLaImagen = tokens.at( indexContentLength + 1 );
                    bytesRestantesDeLaImagenPorLeer = strBytesDeLaImagen.toInt();
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

                        // Este for actualiza el tipo de carnet que solicita analizar este tcpSocket
                        for( int i = 0 ; i < vConexiones.size() ; i++ )  {
                            Conexion * conexion = vConexiones.at( i );
                            if ( conexion->getTcpSocket() == tcpSocket )  {
                                QString tipoCarnet = query.queryItemValue( "carnet" );
                                QString usuario = query.queryItemValue( "usuario" );
                                conexion->setTipo( tipoCarnet );
                                conexion->setUsuario( usuario );
                            }
                        }
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

                QByteArray baData = ba;
                m_baIncomingData.append( baData );
                bytesRestantesDeLaImagenPorLeer -= baData.count();

                ////////////////////////////////////////////////////////
                /// FIN - PASO 2
                /// Solo las tres lineas anteriores se ejecutan para formar la imagen
                ////////////////////////////////////////////////////////


                ////////////////////////////////////////////////////////
                /// PASO 3
                /// Cuando los bytes restantes es cero, entonces se procede a analizar la imagen
                ////////////////////////////////////////////////////////
                if ( bytesRestantesDeLaImagenPorLeer <= 0 )  {

                    if ( im.loadFromData( m_baIncomingData, "JPG" ) )  {
                        im = im.convertToFormat( QImage::Format_RGB888 );

                        QString fecha_hora = QDateTime::currentDateTime().toString( "yyyyMMddhhmmss" );

        #ifdef EJECUTADO_EN_SERVER
                        QString nombreArchivo = "imagenes/" + fecha_hora + ".png";
        #else
                        QString nombreArchivo = "../" + fecha_hora + ".jpg";
        #endif
                        // Guardamos la imagen que el usuario envio para analizar
                        im.save( nombreArchivo.toStdString().c_str() );

                        m_baIncomingData.clear();
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
                    cv::imwrite( "imagenes/registros/paraAlinear.jpg", imParaAlinear );

        #else
                    cv::imwrite( "../../imagenes/registros/paraAlinear.jpg", imParaAlinear );
        #endif

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
                    cv::imwrite( "imagenes/registros/matParaLeerCodigo1.jpg", matParaLeerCodigo );
        #else
                    cv::imwrite( "../../imagenes/registros/matParaLeerCodigo1.jpg", matParaLeerCodigo );
        #endif

                    cv::Mat imAlineada;

                    // Averigua que tipo de carnet solicita analizar este tcpSocket
                    for( int i = 0 ; i < vConexiones.size() ; i++ )  {
                        Conexion * conexion = vConexiones.at( i );
                        if ( conexion->getTcpSocket() == tcpSocket )  {

                            if ( conexion->getTipo() == "dni" )  {

                                // Busca el codigo antes de alinear
                                resultadoZXing = decoder->decodeImage( QImage( matParaLeerCodigo.data, matParaLeerCodigo.cols,
                                                                               matParaLeerCodigo.rows, matParaLeerCodigo.step,
                                                                               QImage::Format_Grayscale8
                                                                             )
                                                                     );

                                Procesador::getInstancia()->alinear( imParaAlinear, imAlineada, Procesador::DNI );
                                qDebug() << "dni";

                                if ( imAlineada.rows == 0 )  {
                                    tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                                    tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                                    tcpSocket->write( "\r\n" );
                                    tcpSocket->write( "{ \"error\":\"No se pudieron extraer correctamente los datos 1.\" }" );
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
                                    cv::imwrite( "imagenes/registros/matParaLeerCodigo2.jpg", matParaLeerCodigo );
        #else
                                    cv::imwrite( "../../imagenes/registros/matParaLeerCodigo2.jpg", matParaLeerCodigo );
        #endif
                                    resultadoZXing = decoder->decodeImage( QImage( matParaLeerCodigo.data, matParaLeerCodigo.cols,
                                                                                   matParaLeerCodigo.rows, matParaLeerCodigo.step,
                                                                                   QImage::Format_Grayscale8
                                                                                 )
                                                                         );

                                }



        #ifdef EJECUTADO_EN_SERVER
                                cv::imwrite( "imagenes/registros/alineada.jpg", imAlineada );

        #else
                                cv::imwrite( "../../imagenes/registros/alineada.jpg", imAlineada );
        #endif


                                cv::Mat imConRectangulos;

                                QStringList datosExtraidos;

                                datosExtraidos = Procesador::getInstancia()->extraerInfo( imAlineada,
                                                                                          imConRectangulos,
                                                                                          Procesador::DNI );

        #ifdef EJECUTADO_EN_SERVER
                                cv::imwrite( "imagenes/registros/conRectangulos.jpg", imConRectangulos );

        #else
                                cv::imwrite( "../../imagenes/registros/conRectangulos.jpg", imConRectangulos );
        #endif


                                cv::Mat imFotoPerfil;

                                Procesador::getInstancia()->extraerFoto( imAlineada, imFotoPerfil, Procesador::DNI );

        #ifdef EJECUTADO_EN_SERVER
                                cv::imwrite( "imagenes/registros/fotoPerfil.jpg", imFotoPerfil );
        #else
                                cv::imwrite( "../../imagenes/registros/fotoPerfil.jpg", imFotoPerfil );
        #endif


                                QString json;

                                bool noSonBuenosLosDatosExtraidos = false;

                                if ( resultadoZXing.isEmpty() )  {

                                    QString dni = datosExtraidos.at( 0 );
                                    dni.remove( QRegExp( "[^a-zA-Z\\d\\s]" ) );  // Elimina signos
                                    dni.replace( "O", "0", Qt::CaseInsensitive );
                                    dni.replace( "I", "1", Qt::CaseInsensitive );  dni.replace( "Z", "2", Qt::CaseInsensitive );

                                    QString apellido = datosExtraidos.at( 1 );
                                    apellido.replace( "0", "O" );  apellido.replace( "1", "I" );
                                    apellido.replace( "|", "I" );  apellido.replace( "2", "Z" );

                                    QString nombre = datosExtraidos.at( 2 );
                                    nombre.replace( "0", "O" );  nombre.replace( "1", "I" );
                                    nombre.replace( "|", "I" );  nombre.replace( "2", "Z" );

                                    QString nacimiento = datosExtraidos.at( 4 );
                                    nacimiento.replace( "O", "0", Qt::CaseInsensitive );
                                    nacimiento.replace( "I", "1", Qt::CaseInsensitive );
                                    nacimiento.replace( "Z", "2", Qt::CaseInsensitive );

                                    QString otorgamiento = datosExtraidos.at( 5 );
                                    otorgamiento.replace( "O", "0", Qt::CaseInsensitive );
                                    otorgamiento.replace( "I", "1", Qt::CaseInsensitive );
                                    otorgamiento.replace( "Z", "2", Qt::CaseInsensitive );

                                    QString vencimiento = datosExtraidos.at( 6 );
                                    vencimiento.replace( "O", "0", Qt::CaseInsensitive );
                                    vencimiento.replace( "I", "1", Qt::CaseInsensitive );
                                    vencimiento.replace( "Z", "2", Qt::CaseInsensitive );

                                    if ( dni.size() != 8 || apellido.size() < 2 || nombre.size() < 2 ||
                                         nacimiento.size() < 4 || otorgamiento.size() < 4 || vencimiento.size() < 4 )
                                        noSonBuenosLosDatosExtraidos = true;

                                    json.append( "{ \"dni\":\"" + dni );
                                    json.append( "\", \"apellido\":\"" + apellido );
                                    json.append( "\", \"nombre\":\"" + nombre );
                                    json.append( "\", \"nacimiento\":\"" + nacimiento );
                                    json.append( "\", \"otorgamiento\":\"" + otorgamiento );
                                    json.append( "\", \"vencimiento\":\"" + vencimiento + "\" }" );
                                }
                                else  {
                                    // 00371082464 @ OSIMANI @ CESAR ALEJANDRO @ M @ 26413920 @ A @ 02/02/1978 @ 21/05/2015 @ 202
                                    //     0            1            2           3       4      5       6            7         8
                                    QStringList datosCodigoPDF147 = resultadoZXing.split( "@" );

                                    json.append( "{ \"dni\":\"" + datosCodigoPDF147.at( 4 ) );
                                    json.append( "\", \"apellido\":\"" + datosCodigoPDF147.at( 1 ) );
                                    json.append( "\", \"nombre\":\"" + datosCodigoPDF147.at( 2 ) );
                                    json.append( "\", \"nacimiento\":\"" + datosCodigoPDF147.at( 6 ) );
                                    json.append( "\", \"otorgamiento\":\"" + datosCodigoPDF147.at( 7 ) );

                                    QDate dateVencimiento = QDate::fromString( datosCodigoPDF147.at( 7 ), "dd/MM/yyyy" ).addYears( 15 );

                                    json.append( "\", \"vencimiento\":\"" + dateVencimiento.toString( "dd/MM/yyyy" ) + "\" }" );
                    //                json.append( "\", \"vencimiento\":\"" + datosCodigoPDF147.at( 8 ) + "\" }" );

                                    if ( datosCodigoPDF147.at( 4 ).size() < 8 )
                                        noSonBuenosLosDatosExtraidos = true;
                                }

                                qDebug() << json;

                                if ( noSonBuenosLosDatosExtraidos )  {
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
                                Procesador::getInstancia()->alinear( imParaAlinear, imAlineada, Procesador::LICENCIA );
                                qDebug() << "licencia";

                                if ( imAlineada.rows == 0 )  {
                                    tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                                    tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                                    tcpSocket->write( "\r\n" );
                                    tcpSocket->write( "{ \"error\":\"No se pudieron extraer correctamente los datos 1.\" }" );
                                    tcpSocket->flush();
                                    tcpSocket->close();

                                    return;
                                }




        #ifdef EJECUTADO_EN_SERVER
                                cv::imwrite( "imagenes/registros/alineada.jpg", imAlineada );

        #else
                                cv::imwrite( "../../imagenes/registros/alineada.jpg", imAlineada );
        #endif


                                cv::Mat imConRectangulos;

                                QStringList datosExtraidos;

                                datosExtraidos = Procesador::getInstancia()->extraerInfo( imAlineada,
                                                                                          imConRectangulos,
                                                                                          Procesador::LICENCIA );

        #ifdef EJECUTADO_EN_SERVER
                                cv::imwrite( "imagenes/registros/conRectangulos.jpg", imConRectangulos );

        #else
                                cv::imwrite( "../../imagenes/registros/conRectangulos.jpg", imConRectangulos );
        #endif


                                cv::Mat imFotoPerfil;

                                Procesador::getInstancia()->extraerFoto( imAlineada, imFotoPerfil, Procesador::LICENCIA );

        #ifdef EJECUTADO_EN_SERVER
                                cv::imwrite( "imagenes/registros/fotoPerfil.jpg", imFotoPerfil );
        #else
                                cv::imwrite( "../../imagenes/registros/fotoPerfil.jpg", imFotoPerfil );
        #endif

                                QString json;

                                QString dni = datosExtraidos.at( 0 );
                                dni.remove( QRegExp( "[^a-zA-Z\\d\\s]" ) );  // Elimina signos
                                dni.replace( "O", "0", Qt::CaseInsensitive );
                                dni.replace( "I", "1", Qt::CaseInsensitive );  dni.replace( "Z", "2", Qt::CaseInsensitive );

                                QString apellido = datosExtraidos.at( 1 );
                                apellido.replace( "0", "O" );  apellido.replace( "1", "I" );
                                apellido.replace( "|", "I" );  apellido.replace( "2", "Z" );

                                QString nombre = datosExtraidos.at( 2 );
                                nombre.replace( "0", "O" );  nombre.replace( "1", "I" );
                                nombre.replace( "|", "I" );  nombre.replace( "2", "Z" );

                                QString nacimiento = datosExtraidos.at( 4 );
                                nacimiento.replace( "O", "0", Qt::CaseInsensitive );
                                nacimiento.replace( "I", "1", Qt::CaseInsensitive );
                                nacimiento.replace( "Z", "2", Qt::CaseInsensitive );

                                QString otorgamiento = datosExtraidos.at( 5 );
                                otorgamiento.replace( "O", "0", Qt::CaseInsensitive );
                                otorgamiento.replace( "I", "1", Qt::CaseInsensitive );
                                otorgamiento.replace( "Z", "2", Qt::CaseInsensitive );

                                QString vencimiento = datosExtraidos.at( 6 );
                                vencimiento.replace( "O", "0", Qt::CaseInsensitive );
                                vencimiento.replace( "I", "1", Qt::CaseInsensitive );
                                vencimiento.replace( "Z", "2", Qt::CaseInsensitive );

                                json.append( "{ \"dni\":\"" + dni );
                                json.append( "\", \"apellido\":\"" + apellido );
                                json.append( "\", \"nombre\":\"" + nombre );
                                json.append( "\", \"nacimiento\":\"" + nacimiento );
                                json.append( "\", \"otorgamiento\":\"" + otorgamiento );
                                json.append( "\", \"vencimiento\":\"" + vencimiento + "\" }" );

                                qDebug() << json;

                                tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                                tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                                tcpSocket->write( "\r\n" );
                                tcpSocket->write( json.toStdString().c_str() );
                                tcpSocket->flush();
                                tcpSocket->close();
                            }
                            else  {
                                qDebug() << "ninguno";
                            }

                        }
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
        tcpSocket->deleteLater();
    }
}



