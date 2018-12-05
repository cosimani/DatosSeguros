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
//    connect( pSocket, &QTcpSocket::textMessageReceived, this, &WebService::processTextMessage );
//    connect( tcpSocket, &QTcpSocket::binaryMessageReceived, this, &WebService::processBinaryMessage );
    connect( tcpSocket, SIGNAL( disconnected() ), this, SLOT( socketDisconnected() ) );

    listTcpSockets << tcpSocket;
}

void WebService::processTextMessage( QString message )  {
    QTcpSocket * tcpSocket = qobject_cast< QTcpSocket * >( sender() );
    qDebug() << "Message received:" << message;

    if ( tcpSocket )  {
        tcpSocket->write( message.toStdString().c_str() );
//        pClient->sendTextMessage( message );
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

    QByteArray ba = tcpSocket->readAll();

//    qDebug() << "SE recibio lo siguienteeeeeeeeeeeeeeeeeee \n " << ba;

//    return;


    QStringList tokens = QString( ba ).split( QRegExp( "\\s+" ) );  // Separa con espacios y lineas nuevas

//    qDebug() << "tokens" << tokens.size();
//    qDebug() << tokens;

    if ( tokens.at( 0 ) == "POST" )  {

        // Este es el index del string donde esta Content-Length
        int indexContentLength = tokens.indexOf( "Content-Length:" );

        // Si Content-Length existe, entonces el string siguiente es el numero de bytes que tiene la imagen
        if ( tokens.size() > indexContentLength )  {
            QString strBytesDeLaImagen = tokens.at( indexContentLength + 1 );
            bytesRestantesDeLaImagenPorLeer = strBytesDeLaImagen.toInt();
            qDebug() << "size de la imagen = " << bytesRestantesDeLaImagenPorLeer;

        }
    }
    else  { // Si entra aca es porque ya solo vienen datos binarios para armar la imagen

//        QByteArray baData = tcpSocket->read( bytesRestantesDeLaImagenPorLeer );
        QByteArray baData = ba;

        qDebug() << "m_baIncomingData size =" << baData.count();

        m_baIncomingData.append( baData );

        bytesRestantesDeLaImagenPorLeer -= baData.count();

        if (bytesRestantesDeLaImagenPorLeer <= 0)
        {

            if ( im.loadFromData( m_baIncomingData, "JPG" ) )
            {
                im = im.convertToFormat( QImage::Format_RGB888 );
                qDebug() << "La imagen se guarda";

                QString fecha_hora = QDateTime::currentDateTime().toString( "yyyyMMddhhmmss" );

#ifdef EJECUTADO_EN_SERVER
                QString nombreArchivo = "imagenes/" + fecha_hora + ".png";
#else
                QString nombreArchivo = "../" + fecha_hora + ".jpg";
#endif

                im.save( nombreArchivo.toStdString().c_str() );
            }

            m_baIncomingData.clear();

    //        emit newImage();
            qDebug() << "emit newImage(); //////////////////////";

//            emit signal_queryItems_and_image( "", QUrlQuery(), im );

            if ( im.isNull() )  {
                tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                tcpSocket->write( "\r\n" );
    //            tcpSocket->write( "OK - " );
    //            tcpSocket->write( QDateTime::currentDateTime().toString().toStdString().c_str() );
    //            tcpSocket->write( "\n" );
                tcpSocket->write( "{ \"error\":\"El server no pudo recibir la imagen correctamente.\" }" );
                tcpSocket->flush();
                tcpSocket->close();

                return;
            }

            cv::Mat imParaAlinear( im.height(),
                                   im.width(),
                                   CV_8UC3,
                                   ( void * )im.constBits(),
                                   im.bytesPerLine()
                                 );

            cv::cvtColor( imParaAlinear, imParaAlinear, cv::COLOR_RGB2BGR );

#ifdef EJECUTADO_EN_SERVER
            cv::imwrite( "imagenes/registros/paraAlinear.jpg", imParaAlinear );

#else
            cv::imwrite( "../../imagenes/registros/paraAlinear.jpg", imParaAlinear );
#endif

            QString resultadoZXing;
            cv::Mat matParaLeerCodigo;
            int umbral = 100;

            cv::cvtColor( imParaAlinear, matParaLeerCodigo, CV_RGB2GRAY );
//            cv::threshold( matParaLeerCodigo, matParaLeerCodigo, umbral, 255, cv::THRESH_BINARY );

//            //Erode the image with 3x3 kernel
//            cv::erode( matParaLeerCodigo, matParaLeerCodigo,
//                       cv::getStructuringElement( cv::MORPH_RECT, cv::Size( 3, 3 ) ) );

            cv::adaptiveThreshold(matParaLeerCodigo, matParaLeerCodigo, 255,
                                  CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY, 91, 15);

#ifdef EJECUTADO_EN_SERVER
            cv::imwrite( "imagenes/registros/matParaLeerCodigo1.jpg", matParaLeerCodigo );
#else
            cv::imwrite( "../../imagenes/registros/matParaLeerCodigo1.jpg", matParaLeerCodigo );
#endif

            resultadoZXing = decoder->decodeImage( QImage( matParaLeerCodigo.data, matParaLeerCodigo.cols,
                                                           matParaLeerCodigo.rows, matParaLeerCodigo.step,
                                                           QImage::Format_Grayscale8
                                                         )
                                                 );


            cv::Mat imAlineada;

//            Procesador::getInstancia()->alinear( imParaAlinear, imAlineada, Procesador::LICENCIA );
            Procesador::getInstancia()->alinear( imParaAlinear, imAlineada, Procesador::DNI );


            if ( imAlineada.rows == 0 )  {
                tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                tcpSocket->write( "\r\n" );
    //            tcpSocket->write( "OK - " );
    //            tcpSocket->write( QDateTime::currentDateTime().toString().toStdString().c_str() );
    //            tcpSocket->write( "\n" );
                tcpSocket->write( "{ \"error\":\"No se pudieron extraer correctamente los datos.\" }" );
                tcpSocket->flush();
                tcpSocket->close();

                return;
            }





            if ( resultadoZXing.isEmpty() )  {
                cv::cvtColor( imAlineada, matParaLeerCodigo, CV_RGB2GRAY );
//                cv::threshold( matParaLeerCodigo, matParaLeerCodigo, umbral, 255, cv::THRESH_BINARY );

                cv::adaptiveThreshold(matParaLeerCodigo, matParaLeerCodigo, 255,
                                      CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY, 91, 15);

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

//            QStringList datosExtraidos = Procesador::getInstancia()->extraerInfo( imAlineada, imConRectangulos, Procesador::LICENCIA );
            QStringList datosExtraidos = Procesador::getInstancia()->extraerInfo( imAlineada, imConRectangulos, Procesador::DNI );


#ifdef EJECUTADO_EN_SERVER
            cv::imwrite( "imagenes/registros/conRectangulos.jpg", imConRectangulos );

#else
            cv::imwrite( "../../imagenes/registros/conRectangulos.jpg", imConRectangulos );
#endif


//            ui->visor->setMat( imConRectangulos );

            cv::Mat imFotoPerfil;

//            Procesador::getInstancia()->extraerFoto( imAlineada, imFotoPerfil, Procesador::LICENCIA );
            Procesador::getInstancia()->extraerFoto( imAlineada, imFotoPerfil, Procesador::DNI );

#ifdef EJECUTADO_EN_SERVER
            cv::imwrite( "imagenes/registros/fotoPerfil.jpg", imFotoPerfil );

#else
            cv::imwrite( "../../imagenes/registros/fotoPerfil.jpg", imFotoPerfil );
#endif


//            ui->visorFotoPerfil->setMat( imFotoPerfil );

//            QThread::msleep( 7000 );

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
    //            tcpSocket->write( "OK - " );
    //            tcpSocket->write( QDateTime::currentDateTime().toString().toStdString().c_str() );
    //            tcpSocket->write( "\n" );
                tcpSocket->write( "{ \"error\":\"No se pudieron extraer correctamente los datos.\" }" );
                tcpSocket->flush();
                tcpSocket->close();

            }
            else  {
                tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
                tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
                tcpSocket->write( "\r\n" );
    //            tcpSocket->write( "OK - " );
    //            tcpSocket->write( QDateTime::currentDateTime().toString().toStdString().c_str() );
    //            tcpSocket->write( "\n" );
                tcpSocket->write( json.toStdString().c_str() );
                tcpSocket->flush();
                tcpSocket->close();

            }



        }
    }


    return;




    // Emite en esta senal todo lo recibido en cada peticion, sin procesar
    emit signal_solicitudRecibida( ba );


    // Algunas validaciones por las dudas
    if ( tokens.size() >= 2 )  {

        QString token1 = tokens.at( 0 );
        QString token2 = tokens.at( 1 );
        QString token3 = tokens.at( 2 );

        if ( ( token1 == "GET" || token1 == "POST" ) && token3.startsWith( "HTTP" ) )  {

            QString recurso_actualizar_php = "actualizar.php";
            QString recurso_grabaciones_php = "grabaciones.php";
            QString recurso_detener_php = "detener.php";
            QString recurso_recibir_image_php = "recibir.php";

            // Entra a este if cuando se recibion una consulta correcta con actualizar.php
            if ( token2.contains( recurso_actualizar_php, Qt::CaseInsensitive ) )  {

                // Si entro a este if, entonces token2 es algo como lo siguiente:
                //      /actualizar.php?q=6&aver=9
                //      /actualizar.php?consulta=usuarios&key=1234

                QStringList recurso_y_variables = token2.split( "?" );

                // Entra si la consulta tiene una o mas variables
                if ( recurso_y_variables.size() >= 2 )  {

                    // query contendra todas las variables para se interpretada con metodos de QUrlQuery
                    QUrlQuery query( recurso_y_variables.at( 1 ) );

                    qDebug() << "Variables de la solicitud" << query.queryItems();

                    // Aca se emiten todas las varibles recibidas en la consulta
                    emit signal_queryItems( recurso_actualizar_php, query );

                }

            }

            // Entra a este if cuando se recibio una consulta correcta con grabaciones.php
            if ( token2.contains( recurso_grabaciones_php, Qt::CaseInsensitive ) )  {

                // Si entro a este if, entonces token2 es algo como lo siguiente:
                //      /grabaciones.php?q=6&aver=9
                //      /grabaciones.php?consulta=usuarios&key=1234

                QStringList recurso_y_variables = token2.split( "?" );

                // Entra si la consulta tiene una o mas variables
                if ( recurso_y_variables.size() >= 2 )  {

                    // query contendra todas las variables para se interpretada con metodos de QUrlQuery
                    QUrlQuery query( recurso_y_variables.at( 1 ) );

                    qDebug() << "Variables de la solicitud" << query.queryItems();

                    // Aca se emiten todas las varibles recibidas en la consulta
                    emit signal_queryItems( recurso_grabaciones_php, query );

                }

            }

            // Entra a este if cuando se recibio una consulta correcta con detener.php
            if ( token2.contains( recurso_detener_php, Qt::CaseInsensitive ) )  {

                // Si entro a este if, entonces token2 es algo como lo siguiente:
                //      /detener.php?confirmar=1

                QStringList recurso_y_variables = token2.split( "?" );

                // Entra si la consulta tiene una o mas variables
                if ( recurso_y_variables.size() >= 2 )  {

                    // query contendra todas las variables para se interpretada con metodos de QUrlQuery
                    QUrlQuery query( recurso_y_variables.at( 1 ) );

                    qDebug() << "Variables de la solicitud" << query.queryItems();

                    // Aca se emiten todas las varibles recibidas en la consulta
                    emit signal_queryItems( recurso_detener_php, query );

                }

            }

            // Entra a este if cuando se recibio una consulta correcta con recibir.php
            if ( token2.contains( recurso_recibir_image_php, Qt::CaseInsensitive ) )  {

                // Si entro a este if, entonces token2 es algo como lo siguiente:
                //      /recibir.php?cliente=cosimani
                // Y en el POST viene el binario de la imagen

                QStringList recurso_y_variables = token2.split( "?" );

                // Entra si la consulta tiene una o mas variables
                if ( recurso_y_variables.size() >= 2 )  {

                    // query contendra todas las variables para se interpretada con metodos de QUrlQuery
                    QUrlQuery query( recurso_y_variables.at( 1 ) );

                    qDebug() << "Variables de la solicitud" << query.queryItems();

                    QImage im;

                    // Aca se emiten todas las varibles recibidas en la consulta y la imagen recibida en el POST
                    emit signal_queryItems_and_image( recurso_recibir_image_php, query, im );


//                    tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
//                    tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
//                    tcpSocket->write( "\r\n" );
//                    tcpSocket->write( "OK - " );
//                    tcpSocket->write( QDateTime::currentDateTime().toString().toStdString().c_str() );
//                    tcpSocket->write( "\n" );
//                    tcpSocket->flush();
//                    tcpSocket->close();

                }

            }


        }
    }


//    if ( tcpSocket )  {


//        tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
//        tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
//        tcpSocket->write( "\r\n" );
//        tcpSocket->write( "OK - " );
//        tcpSocket->write( QDateTime::currentDateTime().toString().toStdString().c_str() );
//        tcpSocket->write( "\n" );
//        tcpSocket->flush();
//        tcpSocket->close();
//    }
}

void WebService::processBinaryMessage( QByteArray message )  {
    QTcpSocket * tcpSocket = qobject_cast< QTcpSocket * >( sender() );
    qDebug() << "Binary Message received:" << message;
//    if ( pClient ) {
//        pClient->sendBinaryMessage( message );
//    }
}

void WebService::socketDisconnected()
{
    QTcpSocket * tcpSocket = qobject_cast< QTcpSocket * >( sender() );
    qDebug() << "socketDisconnected:" << tcpSocket;

    if ( tcpSocket ) {
        listTcpSockets.removeAll( tcpSocket );
        tcpSocket->deleteLater();
    }
}



