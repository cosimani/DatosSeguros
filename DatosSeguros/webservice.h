#ifndef ECHOSERVER_H
#define ECHOSERVER_H

#include <QObject>
#include <QList>
#include <QByteArray>
#include <QStringList>
#include <QUrlQuery>
#include <QImage>

#include <opencv/highgui.h>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

#include <QZXing.h>




class QTcpServer;
class QTcpSocket;


class WebService : public QObject  {
    Q_OBJECT

public:
    explicit WebService( QObject * parent = 0 );
    ~WebService();

    bool iniciar( int puerto );

    bool isEscuchando();

private:
    QTcpServer * tcpServer;
    QList< QTcpSocket * > listTcpSockets;
    int bytesRestantesDeLaImagenPorLeer;
    QByteArray	m_baIncomingData;		// Raw incoming data from socket
    QImage im;
    QZXing * decoder;

private slots:
    void onNewConnection();
    void processTextMessage( QString message );
    void slot_leerDatos();
    void processBinaryMessage( QByteArray message );
    void socketDisconnected();


signals:
    void signal_solicitudRecibida( QByteArray );
    void signal_queryItems( QString, QUrlQuery );
    void signal_queryItems_and_image( QString, QUrlQuery, QImage );
};

#endif //ECHOSERVER_H
