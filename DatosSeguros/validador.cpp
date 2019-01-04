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
    apellido.replace( "0", "O" );  apellido.replace( "1", "I" );  apellido.replace( "!", "I" );
    apellido.replace( "|", "I" );  apellido.replace( "2", "Z" );


    QString nombre = datosExtraidos.at( 2 );
    nombre.replace( "0", "O" );  nombre.replace( "1", "I" );  nombre.replace( "!", "I" );
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
    json.append( "\", \"fecha_nac_dni\":\"" + nacimiento );
    json.append( "\", \"fecha_otorg_dni\":\"" + otorgamiento );
    json.append( "\", \"fecha_venc_dni\":\"" + vencimiento + "\" }" );



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
    json.append( "\", \"fecha_nac_dni\":\"" + datosCodigoPDF147.at( 6 ) );
    json.append( "\", \"fecha_otorg_dni\":\"" + datosCodigoPDF147.at( 7 ) );

    QDate dateVencimiento = QDate::fromString( datosCodigoPDF147.at( 7 ), "dd/MM/yyyy" ).addYears( 15 );

    json.append( "\", \"fecha_venc_dni\":\"" + dateVencimiento.toString( "dd/MM/yyyy" ) + "\" }" );
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

    QString domicilio = datosExtraidos.at( 3 );

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

    QString clase = datosExtraidos.at( 7 );

    json.append( "{ \"dni\":\"" + dni );
    json.append( "\", \"apellido\":\"" + apellido );
    json.append( "\", \"nombre\":\"" + nombre );
    json.append( "\", \"domicilio\":\"" + domicilio );
    json.append( "\", \"fecha_nac_licencia\":\"" + nacimiento );
    json.append( "\", \"fecha_otorg_licencia\":\"" + otorgamiento );
    json.append( "\", \"fecha_venc_licencia\":\"" + vencimiento );
    json.append( "\", \"clase\":\"" + clase + "\" }" );

    return json;
}

QString Validador::validaDniDorso( QStringList datosExtraidos )
{
    // domicilio / lugar_nac / cuil / linea1 / linea2 / linea3
    // o
    // domicilioLinea1 / lineas / domicilioLinea2 / domicilioLinea3 (lugar de nacimiento)

    QString json;


    // Si entra aca es solo porque devuelve domicilioLinea1 / lineas / domicilioLinea2 / domicilioLinea3
    if ( datosExtraidos.size() == 4 )  {
        QString domicilioLinea1 = datosExtraidos.at( 0 );
        domicilioLinea1 = domicilioLinea1.mid( 0, domicilioLinea1.indexOf( "\n" ) );

        QStringList lineasSeparadasDomicilio2 = datosExtraidos.at( 2 ).split( "\n" );

        QString linea2_domicilio;

        if ( lineasSeparadasDomicilio2.size() >= 2 )  {
            linea2_domicilio = lineasSeparadasDomicilio2.at( 1 );
        }

        QStringList lineasSeparadasDomicilio3 = datosExtraidos.at( 3 ).split( "\n" );

        QString linea3_lugar_nac;

        if ( lineasSeparadasDomicilio3.size() >= 2 )  {
            linea3_lugar_nac = lineasSeparadasDomicilio3.at( 2 );
        }


        QString lineas = datosExtraidos.at( 1 );
        lineas.replace( " ", "" );  // Elimina todos los espacios entre la letras.
        lineas.replace( "<K", "<<", Qt::CaseInsensitive );
        lineas.replace( "K<", "<<", Qt::CaseInsensitive );
        lineas.replace( "<X", "<<", Qt::CaseInsensitive );
        lineas.replace( "X<", "<<", Qt::CaseInsensitive );

        QStringList lineasSeparadas = lineas.split( "\n" );

        if ( lineasSeparadas.size() == 4 )  {
            json.append( "{ \"domicilio_dni\":\"" + domicilioLinea1 + " " + linea2_domicilio );
            json.append( "\", \"lugar_nac\":\"" + linea3_lugar_nac );
            json.append( "\", \"linea1\":\"" + lineasSeparadas.at( 0 ) );
            json.append( "\", \"linea2\":\"" + lineasSeparadas.at( 1 ) );
            json.append( "\", \"linea3\":\"" + lineasSeparadas.at( 2 ) + "\" }" );
        }
        else  {
            json.append( "{ \"domicilio_dni\":\"" + domicilioLinea1 );
            json.append( "\", \"lineas\":\"" + lineas + "\" }" );
        }

        return json;
    }

    QString domicilio = datosExtraidos.at( 0 );
    domicilio = domicilio.mid( 0, domicilio.indexOf( "\n" ) );


    QString nacimiento = datosExtraidos.at( 1 ).simplified();

    QString cuil = datosExtraidos.at( 2 ).simplified();

    QString linea1 = datosExtraidos.at( 3 );
    linea1 = linea1.simplified();  // Elimina espacios al principio y fin, y tambien los enter, retorno de carro
    linea1.replace( " ", "" );  // Elimina todos los espacios entre la letras.
    linea1.replace( "<<", "" );  // Elimina todos los espacios entre la letras.

    QString linea2 = datosExtraidos.at( 4 );
    linea2 = linea2.simplified();  // Elimina espacios al principio y fin, y tambien los enter, retorno de carro
    linea2.replace( " ", "" );  // Elimina todos los espacios entre la letras.
    linea2.replace( "<<<", "" );  // Elimina todos los espacios entre la letras.

    QString linea3 = datosExtraidos.at( 5 );
    linea3 = linea3.simplified();  // Elimina espacios al principio y fin, y tambien los enter, retorno de carro
    linea3.replace( " ", "" );  // Elimina todos los espacios entre la letras.
    linea3.replace( "<<<", "" );  // Elimina todos los espacios entre la letras.

    json.append( "{ \"domicilio_dni\":\"" + domicilio );
    json.append( "\", \"nacimiento\":\"" + nacimiento );
    json.append( "\", \"cuil\":\"" + cuil );
    json.append( "\", \"linea1\":\"" + linea1 );
    json.append( "\", \"linea2\":\"" + linea2 );
    json.append( "\", \"linea3\":\"" + linea3 + "\" }" );

    return json;
}

QString Validador::validaLicenciaDorso(QStringList datosExtraidos)
{
    // clase / descripcionClase / descr_observ

    QString json;

    QString clase = datosExtraidos.at( 0 );

    QString descripcionClase = datosExtraidos.at( 1 );
    descripcionClase.replace( "º", "o" );

    QString descr_observ = datosExtraidos.at( 2 );
    descr_observ.replace( "º", "o" );


    json.append( "{ \"clase\":\"" + clase );
    json.append( "\", \"descripcion_clase\":\"" + descripcionClase );
    json.append( "\", \"descr_observ\":\"" + descr_observ + "\" }" );

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

int Validador::levenshtein( const std::string & s1, const std::string & s2 )  {
   int N1 = s1.size();
   int N2 = s2.size();
   int i, j;
   std::vector< int > T( N2 + 1 );

   for ( i = 0 ; i <= N2 ; i++ )
      T[ i ] = i;

   for ( i = 0 ; i < N1 ; i++ )  {
      T[ 0 ] = i + 1;
      int corner = i;

      for ( j = 0 ; j < N2 ; j++ )  {
         int upper = T[ j + 1 ];
         if ( s1[ i ] == s2[ j ] )
            T[ j + 1 ] = corner;
         else
            T[ j + 1 ] = std::min( T[ j ], std::min( upper, corner ) ) + 1;
         corner = upper;
      }
   }
   return T[ N2 ];
}
