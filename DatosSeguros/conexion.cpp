#include "conexion.h"

Conexion::Conexion( QObject * parent ) : QObject( parent ),
                                         tipo( "ninguno" ),
                                         sizeDeLaImagen( 0 ),
                                         bytesRestantesDeLaImagenPorLeer( 0 )
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
