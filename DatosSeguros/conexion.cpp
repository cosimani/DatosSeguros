#include "conexion.h"

Conexion::Conexion( QObject * parent ) : QObject( parent ),
                                         tipo( "ninguno" ),
                                         sizeDeLaImagen( 0 ),
                                         imData( new QByteArray ),
                                         bytesRestantes( 0 )
{

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

