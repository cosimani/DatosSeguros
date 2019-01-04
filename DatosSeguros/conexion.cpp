#include "conexion.h"

Conexion::Conexion( QObject * parent ) : QObject( parent ),
                                         estado( ESPERANDO_POST ),
                                         tcpSocket( NULL ),
                                         tipo( "ninguno" ),
                                         sizeDeLaImagen( 0 ),
                                         imData( new QByteArray ),
                                         bytesRestantes( 0 ),
                                         timer( new QTimer( this ) )

{
    connect( timer, SIGNAL( timeout() ), this, SLOT( slot_cerrarSocket() ) );
}

QTcpSocket *Conexion::getTcpSocket() const
{
    return tcpSocket;
}

void Conexion::setTcpSocket(QTcpSocket *value)
{
    tcpSocket = value;
}

QString Conexion::getTipo() const
{
    return tipo;
}

void Conexion::setTipo(const QString &value)
{
    tipo = value;
}

int Conexion::getSizeDeLaImagen() const
{
    return sizeDeLaImagen;
}

void Conexion::setSizeDeLaImagen(int value)
{
    sizeDeLaImagen = value;
}

QString Conexion::getUsuario() const
{
    return usuario;
}

void Conexion::setUsuario(const QString &value)
{
    usuario = value;
}

int Conexion::getBytesRestantes() const
{
    return bytesRestantes;
}

void Conexion::setBytesRestantes(int value)
{
    bytesRestantes = value;
    timer->start( 5000 );

    if ( bytesRestantes <= 0 )
        timer->stop();
}

QByteArray Conexion::getImData() const
{
    return *imData;
}

void Conexion::appendData(QByteArray value)
{
    imData->append( value );
}

void Conexion::clearData()  {

    bytesRestantes = 0;
    imData->clear();
}

QString Conexion::getFecha_hora_foto() const
{
    return fecha_hora_foto;
}

void Conexion::setFecha_hora_foto(const QString &value)
{
    fecha_hora_foto = value;
}


Conexion::Estado Conexion::getEstado() const
{
    return estado;
}

void Conexion::setEstado(const Estado &value)
{
    estado = value;
}

void Conexion::slot_cerrarSocket()
{
    qDebug() << "bytes prometidos =" << sizeDeLaImagen << " - bytes restantes =" << bytesRestantes;

    if ( tcpSocket && tcpSocket->isOpen() )  {
        tcpSocket->write( "HTTP/1.0 200 OK\r\n" );
        tcpSocket->write( "Content-Type: text/html; charset=\"utf-8\"\r\n" );
        tcpSocket->write( "\r\n" );
        tcpSocket->write( "{ \"error\":\"Socket quedó inactivo, el cliente no envía datos. Se cerró por timeout.\" }" );
        tcpSocket->flush();
        tcpSocket->close();
    }

    timer->stop();
}

