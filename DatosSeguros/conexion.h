#ifndef CONEXION_H
#define CONEXION_H

#include <QObject>

#include <QTcpSocket>
#include <QTimer>

/**
 * @brief The Conexion class Esta clase representa y tiene la informacion de cada una de las conexiones de los
 * clientes. Tiene el QTcpSocket, tiene la info que vino en los parametros, etc.
 */
class Conexion : public QObject
{
    Q_OBJECT
public:
    enum Estado { ESPERANDO_POST, RECIBIENDO_IMAGE, PROCESANDO_IMAGE };

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

    Estado estado;

    Estado getEstado() const;
    void setEstado(const Estado &value);

private:
    QTcpSocket * tcpSocket;
    QString tipo;  // Puede ser dni, licencia, verde (por defecto, ninguno)
    int sizeDeLaImagen;
    QString usuario;
    QByteArray * imData;  // Raw incoming data from socket

    // Aca se almacenan los bytes que faltan leer para completar todos los bytes de la imagen que envia el cliente
    int bytesRestantes;

    QString fecha_hora_foto;

    QTimer * timer;


signals:

public slots:
    void slot_cerrarSocket();
};

#endif // CONEXION_H
