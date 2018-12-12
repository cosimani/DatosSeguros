#include "config.h"
#include <QDebug>

Config* Config::instance = NULL;
QVector< QStringList > * Config::strings = new QVector< QStringList >();

Config::Config( QObject * parent ) : QObject( parent ), ok( false )  {
    init();
}

Config * Config::getInstance()  {
    if( ! instance )  {
        instance = new Config();
    }
    return instance;
}

/**
 * @brief Config::init
 * Se encarga de abrir el archivo de configuracion y mantener en el QVector strings todas las variables
 */
void Config::init()  {
    QString currentPath = QDir::currentPath();
    QString appName = QFileInfo( QCoreApplication::applicationFilePath() ).fileName();


#ifdef EJECUTADO_EN_SERVER

    QString inputFilePath = "config/DatosSeguros.ini";

#else

//    QString inputFilePath = currentPath + CONFIG_DIR + "DatosSeguros" + CONFIG_FILE_EXT;
    QString inputFilePath = "../config/DatosSeguros.ini";

#endif

//    qDebug() << "El archivo .ini es" << inputFilePath;
//    qDebug() << "Raiz actual" << QDir::currentPath();

//    qDebug() << inputFilePath;

    QFile inputFile( inputFilePath );
    if( inputFile.open( QIODevice::ReadOnly ) == false )
    {
//        LOG_ERR( "Config: file not exists: " + inputFilePath );
        qDebug() << "Config: file not exists: " + inputFilePath ;
        return;
    }
    else  {
        this->ok = true;
    }

    QTextStream in( &inputFile );
    while ( ! in.atEnd() )   {
        QString line = in.readLine();

        if( line.contains( CONFIG_FILE_COMMENTS ) )
        {
            line = line.remove(line.indexOf(CONFIG_FILE_COMMENTS),
                               line.length() - line.indexOf(CONFIG_FILE_COMMENTS));
        }

        if(line.simplified().isEmpty()) {
            continue;
        }

        QStringList kv = line.split(CONFIG_FILE_ASSIGNMENT);

        if(kv.size() == 2)
        {
            for(int i = 0; i < 2; i++)
            {
                kv.operator []( i ) = kv.at( i ).simplified();
            }
        }
        strings->append( kv );
    }
    inputFile.close();

//    qDebug() << strings;

}

QString Config::getString( QString key )  {

    // Por las dudas, cada vez que se consulta una variable, leemos el archivo de configuracion de nuevo
    this->init();

    for( int i = 0; i < strings->size(); i++ )      {
        if( strings->at( i ).at(0) == key )  {
            return strings->at( i ).at( 1 );
        }
    }

    LOG_ERR( "Config: string not found: " + key );
    return "";
}

void Config::setString( QString key, QString value )  {

    QString currentPath = QDir::currentPath();
    QString appName = QFileInfo( QCoreApplication::applicationFilePath() ).fileName();
    QString inputFilePath = currentPath + CONFIG_DIR + appName + CONFIG_FILE_EXT;

    QFile inputFile( inputFilePath );
    if( inputFile.open( QIODevice::ReadOnly | QIODevice::Text ) == false )
    {
        LOG_ERR( "Config: file not exists" );
    }

    QTextStream in( &inputFile );

    QStringList lineaPorLinea;

    while ( ! in.atEnd() )  {
        lineaPorLinea << in.readLine();
    }

//    for( int i = 0; i < lineaPorLinea.size(); i++ )  {
//        qDebug() << lineaPorLinea.at( i );
//    }

    inputFile.close();


    // Aca modificamos la o las lineas que necesitemos modificar
    for( int i = 0; i < lineaPorLinea.size(); i++ )  {

        if( lineaPorLinea.at( i ).contains( CONFIG_FILE_COMMENTS ) )
        {
            if( lineaPorLinea.at( i ).simplified().isEmpty() ) {
                continue;
            }
        }

        QStringList kv = lineaPorLinea.at( i ).split( CONFIG_FILE_ASSIGNMENT );

        if( kv.size() == 2 )
        {
            for( int i = 0; i < 2; i++ )
            {
                kv.operator []( i ) = kv.at( i ).simplified();
            }

            // kv.at( 0 ) es donde se almacenan todas las keys
            if ( kv.at( 0 ).contains( key ) )  {
//                qDebug() << "\n\nSe encontro en la linea:" << i << lineaPorLinea.at( i );

                QStringList key_y_valor = lineaPorLinea.at( i ).split( CONFIG_FILE_ASSIGNMENT );

                if ( key_y_valor.size() == 2 )  {
                    QString nuevaLinea = key_y_valor.at( 0 ).simplified()
                            + " " + CONFIG_FILE_ASSIGNMENT + " " + value;
                    lineaPorLinea.replace( i, nuevaLinea );
                }
            }
        }
    }


    QFile outputFile( inputFilePath );
    if( outputFile.open( QIODevice::WriteOnly | QIODevice::Truncate ) == false )
    {
        LOG_ERR( "Config: file not exists" );
    }

    QTextStream out( &outputFile );

    for( int i = 0; i < lineaPorLinea.size(); i++ )  {
        out << lineaPorLinea.at( i ) << "\n";
    }

    outputFile.close();

    // Esto es para que cargue de nuevo los valores leidos desde el archivo, ya que fue modificado en este metodo.
    this->init();

}

bool Config::isOk()
{
    return this->ok;
}


string Config::getStdString(QString key)
{
    return getString(key).toStdString();
}

const char *Config::getCharString(QString key)
{
    return getStdString(key).c_str();
}

Config::~Config()
{
    delete instance;
}
