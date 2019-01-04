#ifndef VALIDADOR_H
#define VALIDADOR_H

#include <QObject>
#include <string>
#include <vector>

class Validador : public QObject
{
    Q_OBJECT

private:
    static Validador * instance;
    explicit Validador( QObject * parent = nullptr );

public:
    static Validador * getInstance();

    QString validarDniDeImagen( QStringList datosExtraidos );
    QString validaDniDeCodigo( QStringList datosCodigoPDF147 );
    QString validarLicenciaDeImagen( QStringList datosExtraidos );
    QString validaDniDorso( QStringList datosExtraidos );
    QString validaLicenciaDorso( QStringList datosExtraidos );
    QString validaVerdeDorso( QStringList datosExtraidos );
    QString validaVerde( QStringList datosExtraidos );

    int levenshtein(const std::string &s1, const std::string &s2);

signals:

public slots:
};

#endif // VALIDADOR_H
