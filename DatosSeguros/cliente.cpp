//#include "cliente.h"
//#include "ui_cliente.h"
//#include <QFileDialog>
//#include <QPainter>
//#include <QDebug>
//#include <QHttpPart>
//#include <QHttpMultiPart>

//Cliente::Cliente(QWidget *parent) : QWidget(parent),
//                                    ui(new Ui::Cliente),
//                                    manager( new QNetworkAccessManager( this ) )
//{
//    ui->setupUi(this);

//    connect( manager, SIGNAL( finished( QNetworkReply * ) ), this, SLOT( slot_descargaFinalizada( QNetworkReply * ) ) );

//    connect( ui->pbElegir, SIGNAL( pressed() ), this, SLOT( slot_elegirFoto() ) );
//    connect( ui->pbEnviar, SIGNAL( pressed() ), this, SLOT( slot_enviar() ) );

//    jpegClient = new CMJPEGClient;

//    connect( jpegClient, SIGNAL(newImage()), this, SLOT(repaint()));

//    im.load( "../../imagenes/DniFoto1.jpg" );

//}

//Cliente::~Cliente()
//{
//    delete ui;
//}

//void Cliente::paintEvent(QPaintEvent *)
//{
//    QPainter painter( this );

//    if ( ! jpegClient->getImage().isNull() )  {
//        QImage im = jpegClient->getImage();
//        painter.drawImage( 0, 0, im.scaled( this->width(), this->height() ) );
//    }
//}

//void Cliente::slot_descargaFinalizada( QNetworkReply * reply )  {
//    qDebug() << "slot_descargaFinalizada" << reply->readAll();
//}

//void Cliente::slot_elegirFoto()
//{
//    QString archivo;
//    archivo = QFileDialog::getOpenFileName( this, "Abrir imagen", "/home/cosimani", "Images (*.png *.xpm *.jpg)");

//    im.load( archivo );

//    this->repaint();



//}

//void Cliente::slot_enviar()
//{

////    m_pClient.connectToHost( "localhost", 4444);

////    // Esperando conexión al servidor HTTP
////    if ( m_pClient.waitForConnected( 20000 ) )
////    {
////        QString sGet = "GET /recibir.php HTTP/1.1\r\n";

////        qDebug() << QString( "Se envio desde el cliente: %1" ).arg( sGet );

////        // Enviando un GET al servidor HTTP
////        m_pClient.write( sGet.toLatin1() );
////    }
////    else
////    {
////        qWarning() << "CMJPEGClient::onDoConnection() : Unable to connect, retrying in 2 seconds";

////        // On retente la connexion dans deux secondes
//////        QTimer::singleShot(2000, this, SLOT(onDoConnection()));
////    }


////    CHTTPServer::CClientData * pData = CHTTPServer::CClientData::getFromSocket( &m_pClient );

//////    if ( pData != nullptr )
////    if ( true )
////    {
////        // Is the socket ready?
////        if ( m_pClient.state() == QTcpSocket::ConnectedState )
////        {
////            qDebug() << "if ( m_pClient->state() == QTcpSocket::ConnectedState )";

////            QByteArray baOutput = CImageUtilities::getInstance()->convertQImageToByteArray( im,
////                                                                                            "JPG",
////                                                                                            -1 );

////            qDebug() << "//////////// iBytesToWrite";
////            qlonglong iBytesToWrite = pData->m_vUserData["BytesToWrite"].toULongLong();



////            // Si la socket a un buffer de sortie suffisament petit
////            if (iBytesToWrite < baOutput.count() * 4)
////            {
////                // Compose the outgoing message :
////                // - Boundary marker
////                // - HTML header
////                // - Image
//////                QByteArray sMessage = getImageDescriptor(baOutput.count()).toLatin1();

////                // HTTP boundary marker compatible with Firefox
////                QString MJPEGBoundaryMarker = "7b3cc56e5f51db803f790dad720ed50a";


////                QByteArray sMessage = QString(
////                                               "--%1\r\n"
////                                               "Content-Type: image/jpeg\r\n"
////                                               "Content-length: %2\r\n"
////                                               "\r\n")
////                                            .arg(MJPEGBoundaryMarker)
////                                            .arg(baOutput.count()).toLatin1();


////                // Write data
////                m_pClient.write(sMessage);
////                m_pClient.write(baOutput);

////                pData->m_vUserData["BytesToWrite"] = iBytesToWrite + (qlonglong) (sMessage.count() + baOutput.count());
////            }

////            if (m_pClient.state() == QTcpSocket::ConnectedState)
////            {
////                // Flush output
////                m_pClient.flush();
////            }
////        }
////    }





////    qDebug() << "jpegClient->openURL( http://localhost:4444 );";
////    jpegClient->openURL( "http://127.0.0.1:4444" );


//    // Fuente: https://philsturgeon.uk/api/2016/01/04/http-rest-api-file-uploads/
//    // Dice que:
////    POST /avatars HTTP/1.1
////    Host: localhost:3000
////    Authentication: Bearer some-token
////    Content-Type: image/jpeg
////    Content-Length: 284

////    raw image content

//    QNetworkRequest req( QUrl( "http://localhost:8888/recibir.php?cliente=cosimani" ) );
////    QNetworkRequest req( QUrl( "http://192.168.1.1:8888/recibir.php?cliente=cosimani" ) );
//    req.setHeader( QNetworkRequest::ContentTypeHeader, "image/jpeg" );
//    req.setHeader( QNetworkRequest::ContentLengthHeader, im.byteCount() );

//    QByteArray baOutput;
//    QBuffer buffer( &baOutput );

//    if ( buffer.open(QIODevice::WriteOnly ) )  {
//        im.save( &buffer, "JPG" );
//        buffer.close();
//    }

//    QNetworkReply * reply = manager->post( req, baOutput );


////    POST /recibir.php?cliente=cosimani HTTP/1.1\r\nHost: localhost:8888\r\nContent-Type: image/jpeg\r\nContent-Length: 1268792\r\nConnection: Keep-Alive\r\nAccept-Encoding: gzip, deflate\r\nAccept-Language: en-US,*\r\nUser-Agent: Mozilla/5.0\r\n\r\n





////    QHttpMultiPart * multiPart = new QHttpMultiPart( QHttpMultiPart::FormDataType );

////    QHttpPart imagePart;
////    imagePart.setHeader( QNetworkRequest::ContentTypeHeader, QVariant( "image/jpeg" ) );
////    imagePart.setHeader( QNetworkRequest::ContentDispositionHeader, QVariant( "form-data; name=\"image\"" ) );
////    QFile * file = new QFile( "../../imagenes/LicenciaFoto5.jpg" );
////    file->open( QIODevice::ReadOnly );
////    imagePart.setBodyDevice( file );
////    file->setParent(multiPart); // we cannot delete the file now, so delete it with the multiPart

////    multiPart->append(imagePart);

////    QUrl url( "http://localhost:8888/recibir.php?cliente=cosimani" );
////    QNetworkRequest request( url );

////    QNetworkReply * reply = manager->post( request, multiPart );

//}
