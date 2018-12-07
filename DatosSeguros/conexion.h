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

private:
    QTcpSocket * tcpSocket;
    QString tipo;  // Puede ser dni, licencia, verde (por defecto, ninguno)
    int sizeDeLaImagen;
    QString usuario;
    QByteArray m_baIncomingData;		// Raw incoming data from socket
    int bytesRestantesDeLaImagenPorLeer;

signals:

public slots:
};

#endif // CONEXION_H
