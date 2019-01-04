#include "webservice.h"
#include <QDebug>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDateTime>
#include <QThread>

#include "procesador.h"
#include "validador.h"
#include "logger.h"
#include "database.h"

WebService::WebService( QObject * parent ) : QObject( parent ),
                                             tcpServer( new QTcpServer( this ) ),
                                             decoder( new QZXing )
{

    decoder->setDecoder( QZXing::DecoderFormat_PDF_417 );

}

WebService::~WebService()
{
    Database::getInstance()->disconnect();

    tcpServer->close();
    qDeleteAll( listTcpSockets.begin(), listTcpSockets.end() );
}

bool WebService::iniciar( int puerto )
{
    bool exito = tcpServer->listen( QHostAddress::Any, puerto );
    if ( exito )  {
        qDebug() << "WebService escuchando en puerto" << puerto;
        LOG_INF( "WebService escuchando en puerto " + QString::number( puerto ) );

        connect( tcpServer, SIGNAL( newConnection() ), this, SLOT( onNewConnection() ) );

        return true;
    }
    else  {
        qDebug() << "EL WebService NOOOO se pudo levantar en puerto" << puerto;
        LOG_ERR( "WebService NOOOO pudo abrir el puerto " + QString::number( puerto ) );
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

    qDebug() << "Conexiones totales = " << vConexiones.size();

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

//            qDebug() << "//////" << ba.mid( 0, 5 );

            QStringList tokens = QString( ba ).split( QRegExp( "\\s+" ) );  // Separa con espacios y lineas nuevas

            if ( tokens.at( 0 ) == "POST" && conexion->getEstado() == Conexion::ESPERANDO_POST )  {
                qDebug() << tokens;
            }
//            qDebug() << tokens;
            /// Lo primero que llega es algo asi:
            /// ("POST", "/recibir.php?cliente=cosimani&carnet=dni", "HTTP/1.1",
            /// "Content-Type:", "image/jpeg",
            /// "Content-Length:", "25107",
            /// "Connection:", "Keep-Alive",
            /// "Accept-Encoding:", "gzip,", "deflate",
            /// "Accept-Language:", "es-AR,en,*",
            /// "User-Agent:", "Mozilla/5.0",
            /// "Host:", "192.168.1.187:8888", "")


            ///////////////////////////////
            /// Un par de validaciones
            ///////////////////////////////

            if ( tokens.at( 0 ) == "GET" || tokens.at( 0 ) == "HEAD" )  {

                QString token2 = tokens.at( 1 );  // token2 es algo como /detener.php?usuario=cosimani&carnet=licencia

                QString detener_php = "detener.php";

                // Entra a este if cuando se recibio una consulta a detener.php. Para cerrar la aplicacion
                if ( token2.contains( detener_php, Qt::CaseInsensitive ) )  {

                    tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                    tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                    tcpSocket->write( "\r\n" );
                    tcpSocket->write( "{ \"info\":\"La aplicación fue cerrada. Tendrá que iniciarla de nuevo.\" }" );
                    tcpSocket->flush();
                    tcpSocket->close();

                    LOG_WAR( "El server fue cerrado con un GET a http://host:8888/detener.php." );
                    qDebug() << "El server fue cerrado con un GET a http://host:8888/detener.php.";

                    emit signal_cerrarAplicacion();

                    return;
                }
                else  {
                    tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                    tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                    tcpSocket->write( "\r\n" );
                    tcpSocket->write( "{ \"info\":\"El server atiende sólo solicitudes POST a recibir.php.\" }" );
                    tcpSocket->flush();
                    tcpSocket->close();

                    LOG_WAR( "El server atiende sólo solucitudes POST." );
                    qDebug() << "El server atiende sólo solucitudes POST.";

                    return;
                }
            }

            ///////////////////////////////
            /// FIN - Un par de validaciones
            ///////////////////////////////




            ////////////////////////////////////////////////////////
            /// PASO 1
            /// Analiza si es POST, extrae Content-Length, extrae carnet, extrae usuario, completa estos datos en
            /// la Conexion correspondiente dentro de vConexiones.
            ////////////////////////////////////////////////////////
            if ( tokens.at( 0 ) == "POST" && conexion->getEstado() == Conexion::ESPERANDO_POST )  {

                if ( tokens.indexOf( "Content-Length:" ) == -1 )  {
                    tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                    tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                    tcpSocket->write( "\r\n" );
                    tcpSocket->write( "{ \"error\":\"Debe enviar el campo Content-Length.\" }" );
                    tcpSocket->flush();
                    tcpSocket->close();

                    LOG_WAR( "Debe enviar el campo Content-Length." );

                    return;
                }

                // Este es el index del string donde esta Content-Length
                int indexContentLength = tokens.indexOf( "Content-Length:" );

                // Si Content-Length existe, entonces el string siguiente es el numero de bytes que tiene la imagen
                if ( tokens.size() > indexContentLength )  {
                    QString strBytesDeLaImagen = tokens.at( indexContentLength + 1 );
                    conexion->setBytesRestantes( strBytesDeLaImagen.toInt() );

                    // Este for actualiza el sizeDeLaImagen de la Conexion que corresponde a este tcpSocket
                    conexion->setSizeDeLaImagen( strBytesDeLaImagen.toInt() );
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

////                        if ( tipoCarnet != "dni" || tipoCarnet != "dni-dorso" || tipoCarnet != "licencia" ||
////                             tipoCarnet != "licencia-dorso" || tipoCarnet != "verde" || tipoCarnet != "verde-dorso" )  {
//                        if ( tipoCarnet != "dni" )  {


//                            tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
//                            tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
//                            tcpSocket->write( "\r\n" );
//                            tcpSocket->write( "{ \"error\":\"La variable carnet no indica las opciones validas.\" }" );
//                            tcpSocket->flush();
//                            tcpSocket->close();

//                            return;
//                        }
                    }
                }

                // Aqui se interpreto la cabecera HTTP. Y es posible que ahi nomas a continuacion comiencen los
                // datos pertenecientes a la imagen. Siempre luego de "\r\n\r\n" que da fin a la cabecera.
                // Entonces eliminamos la cabecera del QByteArray recibido en este pimer paquete recibido.
                // Notar que se setea el estado RECIBIENDO_IMAGE y continuara el if en las proximas lineas.

                QByteArray finDeCabecera( "\r\n\r\n" );
                int inicioCampoDatos = ba.indexOf( finDeCabecera, indexContentLength ) + finDeCabecera.size();
                ba = ba.remove( 0, inicioCampoDatos );


                conexion->setEstado( Conexion::RECIBIENDO_IMAGE );
                // Despues de setear el estado, entrara al siguiente if

            ////////////////////////////////////////////////////////
            /// FIN - PASO 1
            ////////////////////////////////////////////////////////
            }


            // Entra a este if cuando ya se recibio el POST y ahora se esperan que solo vengan los bytes de la imagen
            if ( conexion->getEstado() == Conexion::RECIBIENDO_IMAGE )  {

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


//                if ( conexion->getBytesRestantes() <= 3000 )  {

//                    bool imagenValida = im.loadFromData( conexion->getImData() );
//                    qDebug() << "imagenValida" << imagenValida;

//                }

                if ( conexion->getBytesRestantes() <= 0 )  {

                    bool imagenValida = im.loadFromData( conexion->getImData(), "JPG" );

                    if ( imagenValida )  {
                        im = im.convertToFormat( QImage::Format_RGB888 );

                        QString fecha_hora = QDateTime::currentDateTime().toString( "yyyyMMddhhmmss" );

                        conexion->setFecha_hora_foto( fecha_hora );
                        QString nombreArchivo = "imagenes/registros/" + conexion->getFecha_hora_foto() + ".jpg";

                        // Guardamos la imagen que el usuario envio para analizar
                        im.save( nombreArchivo.toStdString().c_str() );

                        conexion->clearData();

                        if ( im.isNull() )  {
                            tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                            tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                            tcpSocket->write( "\r\n" );
                            tcpSocket->write( "{ \"error\":\"El server no pudo recibir la imagen correctamente.\" }" );
                            tcpSocket->flush();
                            tcpSocket->close();

                            LOG_WAR( "El server no pudo recibir la imagen correctamente." );

                            // este return es porque no se pudo crear la imagen con los datos que envio el usuario
                            return;
                        }

                        // Aca no debemos hacer return porque si no no procesaria la imaen en el if siguiente
                        conexion->setEstado( Conexion::PROCESANDO_IMAGE );

                    }
                    else  {
                        tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                        tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                        tcpSocket->write( "\r\n" );
                        tcpSocket->write( "{ \"error\":\"El server no pudo recibir la imagen correctamente.\" }" );
                        tcpSocket->flush();
                        tcpSocket->close();

                        qDebug() << "El server no pudo recibir la imagen correctamente.";
                        LOG_WAR( "El server no pudo recibir la imagen correctamente." );

                        // este return es porque no se pudo crear la imagen con los datos que envio el usuario
                        return;
                    }
                }
                else  {
                    // Entra a este else cuando se estan recibiendo los bytes para formar la imagen.
                    // Puede pasar aqui que el cliente comience a enviar los bytes pero por algun motivo,
                    // no termine de enviar todos los bytes que prometio en el Content-Length.
                    // Entonces aca activamos un timer que pregunte si los bytes recibidos se siguen incrementando
                    // con el paso de los segundos. Entonces, si pasaron 5 segundos y los bytes recibidos no se
                    // siguen incrementando, entonces finalizamos el socket indicando esto al cliente.

                    // Reemplace esto por un QTimer en Conexion

                }
            }



            if ( conexion->getEstado() == Conexion::PROCESANDO_IMAGE )  {


                ////////////////////////////////////////////////////////
                /// PASO 3
                /// Se procede a analizar la imagen
                ////////////////////////////////////////////////////////


                cv::Mat imParaAlinear( im.height(), im.width(), CV_8UC3, ( void * )im.constBits(), im.bytesPerLine() );

                cv::cvtColor( imParaAlinear, imParaAlinear, cv::COLOR_RGB2BGR );

                QString paraAlinear = "imagenes/registros/" + conexion->getFecha_hora_foto() + "_paraAlinear.jpg";
                cv::imwrite( paraAlinear.toStdString().c_str(), imParaAlinear );


                /////////////////////////////////////////////////////////////
                /// https://stackoverflow.com/questions/24341114/simple-illumination-correction-in-images-opencv-c
                /// simple illumination correction in images openCV c++
                /////////////////////////////////////////////////////////////

                // READ RGB color image and convert it to Lab
                cv::Mat bgr_image = imParaAlinear.clone();
                cv::Mat lab_image;
                cv::cvtColor( bgr_image, lab_image, CV_BGR2Lab );

                // Extract the L channel
                std::vector< cv::Mat > lab_planes( 3 );
                cv::split( lab_image, lab_planes );  // now we have the L image in lab_planes[0]

                // apply the CLAHE algorithm to the L channel
                cv::Ptr< cv::CLAHE > clahe = cv::createCLAHE();
                clahe->setClipLimit( 4 );
                cv::Mat dst;
                clahe->apply( lab_planes[ 0 ], dst );

                // Merge the the color planes back into an Lab image
                dst.copyTo( lab_planes[ 0 ] );
                cv::merge( lab_planes, lab_image );

                // convert back to RGB
                cv::Mat image_clahe;
                cv::cvtColor( lab_image, image_clahe, CV_Lab2BGR );

                // display the results  (you might also want to see lab_planes[0] before and after).
//               cv::imshow("image original", bgr_image);
//               cv::imshow("image CLAHE", image_clahe);


                QString paraclahe = "imagenes/registros/" + conexion->getFecha_hora_foto() + "_clahe.jpg";
                cv::imwrite( paraclahe.toStdString().c_str(), image_clahe );


                if ( conexion->getTipo() == "dni-dorso-busca-todo-el-texto" )  {
                    qDebug() << "dni-dorso";

                    cv::Mat matParaLeerCodigo;
                    cv::cvtColor( imParaAlinear, matParaLeerCodigo, CV_RGB2GRAY );
                    cv::adaptiveThreshold( matParaLeerCodigo, matParaLeerCodigo, 255,
                                           CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY, 91, 15 );

                    QString alineada = "imagenes/registros/" + conexion->getFecha_hora_foto() + "_alineada.jpg";
                    cv::imwrite( alineada.toStdString().c_str(), imParaAlinear );

                    QString datosExtraidos = Procesador::getInstancia()->extraerTexto( imParaAlinear,
                                                                                       Procesador::DNI_DORSO );

                    qDebug() << datosExtraidos;

                    tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                    tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                    tcpSocket->write( "\r\n" );
                    tcpSocket->write( datosExtraidos.toStdString().c_str() );
                    tcpSocket->flush();
                    tcpSocket->close();

                    return;
                }






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

                    QString paraLeerCodigo1 = "imagenes/registros/" +
                                              conexion->getFecha_hora_foto() + "_paraLeerCodigo1.jpg";
                    cv::imwrite( paraLeerCodigo1.toStdString().c_str(), matParaLeerCodigo );

                    cv::Mat imAlineada;

                    // Busca el codigo antes de alinear
                    resultadoZXing = decoder->decodeImage( QImage( matParaLeerCodigo.data, matParaLeerCodigo.cols,
                                                                   matParaLeerCodigo.rows, matParaLeerCodigo.step,
                                                                   QImage::Format_Grayscale8 ) );


                    bool exito = Procesador::getInstancia()->alinearSurfFlann( imParaAlinear,
                                                                               imAlineada, Procesador::DNI );

                    if ( ! exito )  {
                        tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                        tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                        tcpSocket->write( "\r\n" );
                        tcpSocket->write( "{ \"error\":\"No se pudo alinear la imagen (DNI frente). Intente de nuevo!\" }" );
                        tcpSocket->flush();
                        tcpSocket->close();

                        LOG_WAR( "No se pudo alinear la imagen del DNI frente. Intente de nuevo!" );

                        return;
                    }


                    if ( imAlineada.rows == 0 )  {
                        tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                        tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                        tcpSocket->write( "\r\n" );
                        tcpSocket->write( "{ \"error\":\"Parece que no hay un DNI de frente en la imagen.\" }" );
                        tcpSocket->flush();
                        tcpSocket->close();

                        LOG_WAR( "Parece que no hay un DNI de frente en la imagen." );

                        return;
                    }

                    if ( resultadoZXing.isEmpty() )  {
                        cv::cvtColor( imAlineada, matParaLeerCodigo, CV_RGB2GRAY );
        //                cv::threshold( matParaLeerCodigo, matParaLeerCodigo, umbral, 255, cv::THRESH_BINARY );

                        cv::adaptiveThreshold( matParaLeerCodigo, matParaLeerCodigo, 255,
                                               CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY, 91, 15 );

                        QString paraLeerCodigo2 = "imagenes/registros/" +
                                                  conexion->getFecha_hora_foto() + "_paraLeerCodigo2.jpg";
                        cv::imwrite( paraLeerCodigo2.toStdString().c_str(), matParaLeerCodigo );

                        resultadoZXing = decoder->decodeImage( QImage( matParaLeerCodigo.data,
                                                                       matParaLeerCodigo.cols,
                                                                       matParaLeerCodigo.rows,
                                                                       matParaLeerCodigo.step,
                                                                       QImage::Format_Grayscale8 ) );
                    }

                    QString alineada = "imagenes/registros/" + conexion->getFecha_hora_foto() + "_alineada.jpg";
                    cv::imwrite( alineada.toStdString().c_str(), imAlineada );

                    cv::Mat imConRectangulos;

                    QStringList datosExtraidos = Procesador::getInstancia()->extraerInfo( imAlineada,
                                                                                          imConRectangulos,
                                                                                          Procesador::DNI );

                    QString conRectangulos = "imagenes/registros/" +
                                             conexion->getFecha_hora_foto() + "_conRectangulos.jpg";
                    cv::imwrite( conRectangulos.toStdString().c_str(), imConRectangulos );


                    cv::Mat imFotoPerfil;

                    Procesador::getInstancia()->extraerFoto( imAlineada, imFotoPerfil, Procesador::DNI );

                    QString fotoPerfil = "imagenes/registros/" + conexion->getFecha_hora_foto() + "_fotoPerfil.jpg";
                    cv::imwrite( fotoPerfil.toStdString().c_str(), imFotoPerfil );

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

                        LOG_WAR( "No se pudieron extraer correctamente los datos." );
                    }
                    else  {

                        // Para crear el json se puede hacer:
                        // QJsonObject object
//                            {
//                                {"property1", 1},
//                                {"property2", 2}
//                            };


                        tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                        tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                        tcpSocket->write( "\r\n" );
                        tcpSocket->write( json.toStdString().c_str() );
                        tcpSocket->flush();
                        tcpSocket->close();

                        Database::getInstance()->procesarJson( json );

                        LOG_INF( "Datos extraidos = " + json );
                    }
                }
                else if ( conexion->getTipo() == "dni-dorso" )  {
                    qDebug() << "dni-dorso";

                    cv::Mat imAlineada;

                    bool exito = Procesador::getInstancia()->alinearSurfFlann( imParaAlinear,
                                                                               imAlineada, Procesador::DNI_DORSO );

                    if ( ! exito )  {
                        tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                        tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                        tcpSocket->write( "\r\n" );
                        tcpSocket->write( "{ \"error\":\"No se pudo alinear la imagen (DNI dorso). Intente de nuevo!\" }" );
                        tcpSocket->flush();
                        tcpSocket->close();

                        LOG_WAR( "No se pudo alinear la imagen del DNI dorso. Intente de nuevo!" );

                        return;
                    }


                    if ( imAlineada.rows == 0 )  {
                        tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                        tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                        tcpSocket->write( "\r\n" );
                        tcpSocket->write( "{ \"error\":\"Parece que no hay un DNI de dorso en la imagen.\" }" );
                        tcpSocket->flush();
                        tcpSocket->close();

                        LOG_WAR( "Parece que no hay un DNI dorso en la imagen." );

                        return;
                    }


                    QString alineada = "imagenes/registros/" + conexion->getFecha_hora_foto() + "_alineada.jpg";
                    cv::imwrite( alineada.toStdString().c_str(), imAlineada );

                    cv::Mat imConRectangulos;

                    QStringList datosExtraidos = Procesador::getInstancia()->extraerInfo( imAlineada,
                                                                                          imConRectangulos,
                                                                                          Procesador::DNI_DORSO );

                    QString conRectangulos = "imagenes/registros/" +
                                             conexion->getFecha_hora_foto() + "_conRectangulos.jpg";
                    cv::imwrite( conRectangulos.toStdString().c_str(), imConRectangulos );

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

                    cv::Mat imAlineada;

                    bool exito = Procesador::getInstancia()->alinearSurfFlann( imParaAlinear,
                                                                               imAlineada, Procesador::LICENCIA );

                    if ( ! exito )  {
                        tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                        tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                        tcpSocket->write( "\r\n" );
                        tcpSocket->write( "{ \"error\":\"No se pudo alinear la imagen (licencia frente).\" }" );
                        tcpSocket->flush();
                        tcpSocket->close();

                        LOG_WAR( "No se pudo alinear la imagen de la licencia de frente." );

                        return;
                    }


                    if ( imAlineada.rows == 0 )  {
                        tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                        tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                        tcpSocket->write( "\r\n" );
                        tcpSocket->write( "{ \"error\":\"Parece que no hay una licencia de frente en la imagen.\" }" );
                        tcpSocket->flush();
                        tcpSocket->close();

                        LOG_WAR( "Parece que no hay una licencia de frente en la imagen." );

                        return;
                    }


                    QString alineada = "imagenes/registros/" + conexion->getFecha_hora_foto() + "_alineada.jpg";
                    cv::imwrite( alineada.toStdString().c_str(), imAlineada );

                    cv::Mat imConRectangulos;

                    QStringList datosExtraidos = Procesador::getInstancia()->extraerInfo( imAlineada,
                                                                                          imConRectangulos,
                                                                                          Procesador::LICENCIA );

                    QString conRectangulos = "imagenes/registros/" + conexion->getFecha_hora_foto() + "_conRectangulos.jpg";
                    cv::imwrite( conRectangulos.toStdString().c_str(), imConRectangulos );

                    QString json = Validador::getInstance()->validarLicenciaDeImagen( datosExtraidos );

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
                else if ( conexion->getTipo() == "licencia-dorso" )  {
                    qDebug() << "licencia-dorso";

                    cv::Mat imAlineada;

                    bool exito = Procesador::getInstancia()->alinearSurfFlann( imParaAlinear,
                                                                               imAlineada, Procesador::LICENCIA_DORSO );

                    if ( ! exito )  {
                        tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                        tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                        tcpSocket->write( "\r\n" );
                        tcpSocket->write( "{ \"error\":\"No se pudo alinear la imagen (licencia dorso).\" }" );
                        tcpSocket->flush();
                        tcpSocket->close();

                        LOG_WAR( "No se pudo alinear la imagen de la licencia de dorso." );

                        return;
                    }


                    if ( imAlineada.rows == 0 )  {
                        tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                        tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                        tcpSocket->write( "\r\n" );
                        tcpSocket->write( "{ \"error\":\"Parece que no hay una licencia de dorso en la imagen.\" }" );
                        tcpSocket->flush();
                        tcpSocket->close();

                        LOG_WAR( "Parece que no hay una licencia de dorso en la imagen." );

                        return;
                    }


                    QString alineada = "imagenes/registros/" + conexion->getFecha_hora_foto() + "_alineada.jpg";
                    cv::imwrite( alineada.toStdString().c_str(), imAlineada );

                    cv::Mat imConRectangulos;

                    QStringList datosExtraidos = Procesador::getInstancia()->extraerInfo( imAlineada,
                                                                                          imConRectangulos,
                                                                                          Procesador::LICENCIA_DORSO );

                    QString conRectangulos = "imagenes/registros/" + conexion->getFecha_hora_foto() + "_conRectangulos.jpg";
                    cv::imwrite( conRectangulos.toStdString().c_str(), imConRectangulos );

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

                    bool exito = Procesador::getInstancia()->alinearSurfFlann( imParaAlinear,
                                                                               imAlineada, Procesador::VERDE );

                    if ( ! exito )  {
                        tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                        tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                        tcpSocket->write( "\r\n" );
                        tcpSocket->write( "{ \"error\":\"No se pudo alinear la imagen. Intente de nuevo!\" }" );
                        tcpSocket->flush();
                        tcpSocket->close();

                        return;
                    }


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

                    bool exito = Procesador::getInstancia()->alinearSurfFlann( imParaAlinear,
                                                                               imAlineada, Procesador::VERDE_DORSO );

                    if ( ! exito )  {
                        tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                        tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                        tcpSocket->write( "\r\n" );
                        tcpSocket->write( "{ \"error\":\"No se pudo alinear la imagen. Intente de nuevo!\" }" );
                        tcpSocket->flush();
                        tcpSocket->close();

                        return;
                    }


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
