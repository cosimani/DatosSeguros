#include "logger.h"

Logger* Logger::instance = NULL;

QString Logger::out = CONSOLE;

QFile* Logger::file = NULL;

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

QString Logger::getOut() const
{
    return out;
}

void Logger::setPrefixArchivoLog( const QString &value )
{
    out = value;

    if( getFile() == NULL )  {
        QString fecha_hora = QDateTime::currentDateTime().toString( "yyyyMMddhhmmss" );

        file = new QFile( getOut() + fecha_hora + ".log");

        if( ! getFile()->isOpen() )
        {
            getFile()->open( QIODevice::ReadWrite );
        }

        if( getFile()->isOpen() )  {
            LOG_INF( QString( "Logger: logging to " + file->fileName() ) );
        }
        else  {
            LOG_INF( QString( "Logger: No se pudo abrir el archivo " + file->fileName() ) );
        }
    }
}

QFile *Logger::getFile() const
{
    return file;
}

void Logger::setFile(QFile *value)
{
    file = value;
}

void Logger::log( QString text )
{
    if( getOut() == CONSOLE )  {
        qDebug() << text;
        return;
    }

    if( getFile()->isOpen() )
    {
        QTextStream stream( getFile() );
        stream << text << endl;
    }
    else
    {
        setPrefixArchivoLog( CONSOLE );
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
