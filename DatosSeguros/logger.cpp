#include "logger.h"

Logger* Logger::instance = NULL;

QString Logger::out = CONSOLE;

QFile * Logger::file = NULL;

Logger::Logger(QObject *parent) :
    QObject(parent)
{
}

Logger * Logger::getInstance()
{
    if( ! instance )
    {
        instance = new Logger();
    }
    return instance;
}

void Logger::setPrefixArchivoLog( const QString &value )
{
    out = value;

    if( file == NULL )  {
        QString fecha_hora = QDateTime::currentDateTime().toString( "yyyyMMddhhmmss" );

        file = new QFile( out + fecha_hora + ".log");

        if( ! file->isOpen() )
        {
            file->open( QIODevice::ReadWrite );
        }

        if( file->isOpen() )  {
            LOG_INF( QString( "Logger: logging to " + file->fileName() ) );
            qDebug() << "Logger: logging to " + file->fileName();
        }
        else  {
            LOG_INF( QString( "Logger: No se pudo abrir el archivo " + file->fileName() ) );
            qDebug() << "Logger: No se pudo abrir el archivo " + file->fileName();
        }
    }
}

void Logger::log( QString text )
{
    if( out == CONSOLE )  {
        qDebug() << text;
        return;
    }

    if( file->isOpen() )
    {
        QTextStream stream( file );
        stream << text << endl;
    }
    else
    {
        this->setPrefixArchivoLog( CONSOLE );
        log( text );
    }
}

void Logger::inf( QString text )
{
    log( QString( PREF_INF + text ) );

    if( Database::getInstance()->databaseOk )  {
        Database::getInstance()->log( text, LINF );
    }
}

void Logger::war( QString text )
{
    log( QString( PREF_WAR + text ) );

    if( Database::getInstance()->databaseOk )  {
        Database::getInstance()->log( text, LWAR );
    }
}

void Logger::err( QString text )
{
    log( QString( PREF_ERR + text ) );

   if( Database::getInstance()->databaseOk )  {
       Database::getInstance()->log( text, LERR );
   }

   throw new runtime_error( text.toStdString() );
}

Logger::~Logger()
{
    file->close();
    delete file;
    delete instance;
}
