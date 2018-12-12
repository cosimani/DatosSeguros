#ifndef CONEXION_H
#define CONEXION_H

#include <QObject>

#include <QTcpSocket>

/**
 * @brief The Conexion class Esta clase representa y tiene la informacion de cada una de las conexiones de los
 * clientes. Tiene el QTcpSocket, tiene la info que vino en los parametros, etc.
 */
class Conexion : public QObject
{
    Q_OBJECT
public:
    explicit Conexion(QObject *parent = nullptr);

    QTcpSocket *getTcpSocket() const;
    void setTcpSocket(QTcpSocket *value);

    QString getTipo() const;
    void setTipo(const QString &value);

    int getSizeDeLaImagen() const;
    void setSizeDeLaImagen(int value);

    QString getUsuario() const;
    void setUsuario(const QString &value);

    int getBytesRestantes() const;
    void setBytesRestantes(int value);

    QByteArray getImData() const;
    void appendData(QByteArray value);

    void clearData();

    QString getFecha_hora_foto() const;
    void setFecha_hora_foto(const QString &value);

private:
    QTcpSocket * tcpSocket;
    QString tipo;  // Puede ser dni, licencia, verde (por defecto, ninguno)
    int sizeDeLaImagen;
    QString usuario;
    QByteArray * imData;  // Raw incoming data from socket

    // Aca se almacenan los bytes que faltan leer para completar todos los bytes de la imagen que envia el cliente
    int bytesRestantes;

    QString fecha_hora_foto;

signals:

public slots:
};

#endif // CONEXION_H
