#include "validador.h"
#include <QDate>
#include <QDebug>

Validador * Validador::instance = NULL;

Validador::Validador(QObject *parent) : QObject(parent)
{

}

Validador * Validador::getInstance()  {
    if( ! instance )  {
        instance = new Validador;
    }
    return instance;
}

QString Validador::validarDniDeImagen( QStringList datosExtraidos )
{
    QString json;

    QString dni = datosExtraidos.at( 0 );
    dni.remove( QRegExp( "[^a-zA-Z\\d\\s]" ) );  // Elimina signos
    dni.replace( "O", "0", Qt::CaseInsensitive );
    dni.replace( "I", "1", Qt::CaseInsensitive );
    dni.replace( "Z", "2", Qt::CaseInsensitive );

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

//    if ( dni.size() != 8 || apellido.size() < 2 || nombre.size() < 2 ||
//         nacimiento.size() < 4 || otorgamiento.size() < 4 || vencimiento.size() < 4 )
//        return QString();  // Devuelve un QString vacio cuando los datos son malos

    json.append( "{ \"dni\":\"" + dni );
    json.append( "\", \"apellido\":\"" + apellido );
    json.append( "\", \"nombre\":\"" + nombre );
    json.append( "\", \"nacimiento\":\"" + nacimiento );
    json.append( "\", \"otorgamiento\":\"" + otorgamiento );
    json.append( "\", \"vencimiento\":\"" + vencimiento + "\" }" );

    return json;
}

QString Validador::validaDniDeCodigo( QStringList datosCodigoPDF147 )
{
    QString json;

    qDebug() << datosCodigoPDF147;

    // 00371082464 @ OSIMANI @ CESAR ALEJANDRO @ M @ 26413920 @ A @ 02/02/1978 @ 21/05/2015 @ 202
    //     0            1            2           3       4      5       6            7         8

    json.append( "{ \"dni\":\"" + datosCodigoPDF147.at( 4 ) );
    json.append( "\", \"apellido\":\"" + datosCodigoPDF147.at( 1 ) );
    json.append( "\", \"nombre\":\"" + datosCodigoPDF147.at( 2 ) );
    json.append( "\", \"nacimiento\":\"" + datosCodigoPDF147.at( 6 ) );
    json.append( "\", \"otorgamiento\":\"" + datosCodigoPDF147.at( 7 ) );

    QDate dateVencimiento = QDate::fromString( datosCodigoPDF147.at( 7 ), "dd/MM/yyyy" ).addYears( 15 );

    json.append( "\", \"vencimiento\":\"" + dateVencimiento.toString( "dd/MM/yyyy" ) + "\" }" );
//                json.append( "\", \"vencimiento\":\"" + datosCodigoPDF147.at( 8 ) + "\" }" );

    if ( datosCodigoPDF147.at( 4 ).size() == 8 )
        return json;
    else
        return QString();
}

QString Validador::validarLicenciaDeImagen( QStringList datosExtraidos )
{
    QString json;

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

    json.append( "{ \"dni\":\"" + dni );
    json.append( "\", \"apellido\":\"" + apellido );
    json.append( "\", \"nombre\":\"" + nombre );
    json.append( "\", \"nacimiento\":\"" + nacimiento );
    json.append( "\", \"otorgamiento\":\"" + otorgamiento );
    json.append( "\", \"vencimiento\":\"" + vencimiento + "\" }" );

    return json;
}

QString Validador::validaDniDorso( QStringList datosExtraidos )
{
    // Domicilio / Nacimiento / Cuil / Linea1 / Linea2 / Linea3

    QString json;

    QString domicilio = datosExtraidos.at( 0 );

    QString nacimiento = datosExtraidos.at( 1 );

    QString cuil = datosExtraidos.at( 2 );

    QString linea1 = datosExtraidos.at( 3 );

    QString linea2 = datosExtraidos.at( 4 );

    QString linea3 = datosExtraidos.at( 5 );

    json.append( "{ \"domicilio\":\"" + domicilio );
    json.append( "\", \"nacimiento\":\"" + nacimiento );
    json.append( "\", \"cuil\":\"" + cuil );
    json.append( "\", \"linea1\":\"" + linea1 );
    json.append( "\", \"linea2\":\"" + linea2 );
    json.append( "\", \"linea3\":\"" + linea3 + "\" }" );

    return json;
}

QString Validador::validaLicenciaDorso(QStringList datosExtraidos)
{
    // DescripcionLinea1 / DescripcionLinea2 / Donante / Sangre / Cuil / Observaciones / Restriccion

    QString json;

    QString descripcionLinea1 = datosExtraidos.at( 0 );

    QString descripcionLinea2 = datosExtraidos.at( 1 );

    QString donante = datosExtraidos.at( 2 );

    QString sangre = datosExtraidos.at( 3 );

    QString cuil = datosExtraidos.at( 4 );

    QString observaciones = datosExtraidos.at( 5 );

    QString restriccion = datosExtraidos.at( 6 );

    json.append( "{ \"descripcionLinea1\":\"" + descripcionLinea1 );
    json.append( "\", \"descripcionLinea2\":\"" + descripcionLinea2 );
    json.append( "\", \"donante\":\"" + donante );
    json.append( "\", \"sangre\":\"" + sangre );
    json.append( "\", \"cuil\":\"" + cuil );
    json.append( "\", \"observaciones\":\"" + observaciones );
    json.append( "\", \"restriccion\":\"" + restriccion + "\" }" );

    return json;
}

QString Validador::validaVerdeDorso(QStringList datosExtraidos)
{
    // Titular / Dni / Domicilio / Localidad

    QString json;

    QString titular = datosExtraidos.at( 0 );

    QString dni = datosExtraidos.at( 1 );

    QString domicilio = datosExtraidos.at( 2 );

    QString localidad = datosExtraidos.at( 3 );

    json.append( "{ \"titular\":\"" + titular );
    json.append( "\", \"dni\":\"" + dni );
    json.append( "\", \"domicilio\":\"" + domicilio );
    json.append( "\", \"localidad\":\"" + localidad + "\" }" );

    return json;

}

QString Validador::validaVerde(QStringList datosExtraidos)
{
    // Dominio / Modelo / Tipo / Uso / Chasis / Motor / Vencimiento / Marca

    QString json;

    QString dominio = datosExtraidos.at( 0 );

    QString modelo = datosExtraidos.at( 1 );

    QString tipo = datosExtraidos.at( 2 );

    QString uso = datosExtraidos.at( 3 );

    QString chasis = datosExtraidos.at( 4 );

    QString motor = datosExtraidos.at( 5 );

    QString vencimiento = datosExtraidos.at( 6 );

    QString marca = datosExtraidos.at( 7 );

    json.append( "{ \"dominio\":\"" + dominio );
    json.append( "\", \"modelo\":\"" + modelo );
    json.append( "\", \"tipo\":\"" + tipo );
    json.append( "\", \"uso\":\"" + uso );
    json.append( "\", \"chasis\":\"" + chasis );
    json.append( "\", \"motor\":\"" + motor );
    json.append( "\", \"vencimiento\":\"" + vencimiento );
    json.append( "\", \"marca\":\"" + marca + "\" }" );

    return json;

}
