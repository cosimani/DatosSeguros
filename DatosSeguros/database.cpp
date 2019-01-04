#include "database.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

Database* Database::instance = NULL;

Database::Database( QObject *parent ) : QObject( parent ),
                                        databaseOk( false )
{
    database = QSqlDatabase::addDatabase( Config::getInstance()->getString( DATABASE_DRIVER ) );
}

QString Database::getLoginQuery()
{
    return Config::getInstance()->getString( LOGIN_QUERY );
}

QString Database::getSessionQuery()
{
    return Config::getInstance()->getString( SESSION_QUERY );
}

QString Database::getPlateDetectionQuery()
{
    return Config::getInstance()->getString( PLATE_DETECTION_QUERY );
}

QString Database::getCandidateQuery()
{
    return Config::getInstance()->getString( CANDIDATE_QUERY );
}

QString Database::getLogQuery()
{
    return Config::getInstance()->getString( LOG_QUERY );
}

Database *Database::getInstance()
{
    if( instance == NULL )
    {
        instance = new Database();
        LOG_INF( "Database: successfully initialized" );
    }
    return instance;
}

void Database::connect()
{
    Database::database.setDatabaseName( Config::getInstance()->getString( DATABASE_URI ) );

    if( database.open() == false )
    {
        qDebug() << "No se pudo abrir la base de datos. No se puede continuar.";
        LOG_ERR( "Database: cannot open connection" );
    }

    LOG_INF( "Database: connected ok" );

    databaseOk = true;
}

void Database::disconnect()
{
    if( database.isOpen() )
    {
        database.close();
        LOG_INF( "Database: disconnected ok" );
    }
}

int Database::login( QString user, QString password )
{
    QString encryptedPassword =
            QString(QCryptographicHash::hash(
                        QString(password).toUtf8(),
                        QCryptographicHash::Md5).toHex());

    QSqlQuery query;

    query.prepare(getLoginQuery());
    query.bindValue(":user", user);
    query.bindValue(":password", encryptedPassword);

    if(query.exec() == false)
    {
        LOG_ERR(QString("Database: " + query.lastError().text()));
        return -1;
    }

    if(query.next())
    {
        int result = query.value(0).toInt();

        switch(result)
        {
        case 0:
            LOG_WAR(QString("Database: invalid username or password [" + user + ":" + encryptedPassword +  "]"));
            break;

        default:
            if(result > 0) {
                LOG_INF("Database: loggin successful");
            }
        }
        return result;
    }

    LOG_WAR("Database: unexpected login query result");
    return -1;
}

int Database::startSession( int adminId )
{
    QSqlQuery query;

    query.prepare( getSessionQuery() );
    query.bindValue( ":adminId", adminId );

    if( query.exec() == false )
    {
        LOG_ERR( QString( "Database: " + query.lastError().text() ) );
        return -1;
    }

    return query.lastInsertId().toInt();
}

int Database::plateDetection(int sessionId)
{
    QSqlQuery query;

    query.prepare(getPlateDetectionQuery());
    query.bindValue(":sessionId", sessionId);

    if(query.exec() == false)
    {
        LOG_ERR(QString("Database: " + query.lastError().text()));
        return -1;
    }

    return query.lastInsertId().toInt();
}

int Database::nuevaPersona( QString dni, QString apellido, QString nombre, QString sexo )
{
    QSqlQuery query;

    query.prepare( Config::getInstance()->getString( "persona_insert_query" ) );
    query.bindValue( ":dni", dni );
    query.bindValue( ":apellido", apellido );
    query.bindValue( ":nombre", nombre );
    query.bindValue( ":sexo", sexo );

    if( query.exec() == false )
    {
        LOG_ERR( QString( "Database: " + query.lastError().text() ) );
        return -1;
    }

    LOG_INF( QString( "Se almacenó en la base: " + dni + " " + apellido + " " + nombre + " " + sexo ) );

    qDebug() << query.lastQuery();

    return query.lastInsertId().toInt();
}

int Database::actualizarPersona( QString dni, QString apellido, QString nombre, QString sexo )
{
    QSqlQuery query;

    query.prepare( Config::getInstance()->getString( "persona_update_query" ) );

    query.bindValue( ":dni", dni );
    query.bindValue( ":apellido", apellido );
    query.bindValue( ":nombre", nombre );
    query.bindValue( ":sexo", sexo );

    if( query.exec() == false )
    {
        LOG_ERR( QString( "Database: " + query.lastError().text() ) );
        return -1;
    }

    LOG_INF( QString( "Se actualizó en la base: " + dni + " " + apellido + " " + nombre + " " + sexo ) );

    qDebug() << query.lastQuery();

    return query.lastInsertId().toInt();
}

QVector<QStringList> Database::todasLasPersonas()
{
    QVector< QStringList > v;

    QSqlQuery query;

    query.prepare( Config::getInstance()->getString( "persona_select_query_todos" ) );

    if( query.exec() == false )
    {
        LOG_ERR( QString( "Database: " + query.lastError().text() ) );
        return v;
    }

    while ( query.next() )  {
        QSqlRecord rec = query.record();
        QStringList campos;
        int i = 0;
        while ( i < rec.count() )  {
            campos << rec.value( i ).toString();
            i++;
        }

        v.append( campos );
    }

    return v;
}

bool Database::procesarJson( QString json )
{
    QJsonDocument doc( QJsonDocument::fromJson( json.toUtf8() ) );
    QJsonObject jsonObject = doc.object();

    QString dni;
    QString apellido;
    QString nombre;
    QString sexo;
    QString ejemplar;
    QString nacionalidad;
    QString fecha_nac;
    QString domicilio_dni;
    QString domicilio_licencia;
    QString fecha_otorg_dni;
    QString fecha_otorg_licencia;
    QString fecha_venc_dni;
    QString fecha_venc_licencia;
    QString cuil;
    QString clase_licencia;
    QString is_donante;
    QString sangre;
    QString observaciones_licecnia;
    QString restriccion_licencia;
    QString lugar_entrega_licencia;


    foreach( const QString & key, jsonObject.keys() )  {
        QJsonValue value = jsonObject.value( key );
        qDebug() << "Key = " << key << ", Value = " << value.toString();
    }

//    QSqlQuery query;

//    query.prepare( Config::getInstance()->getString( "persona_update_query" ) );

//    query.bindValue( ":dni", dni );
//    query.bindValue( ":apellido", apellido );
//    query.bindValue( ":nombre", nombre );
//    query.bindValue( ":sexo", sexo );

//    if( query.exec() == false )
//    {
//        LOG_ERR( QString( "Database: " + query.lastError().text() ) );
//        return false;
//    }

//    LOG_INF( QString( "Se actualizó en la base: " + dni + " " + apellido + " " + nombre + " " + sexo ) );

//    qDebug() << query.lastQuery();

    return true;
}


int Database::log( QString text, int level )  {
    QSqlQuery query;

    query.prepare( getLogQuery() );
    query.bindValue( ":m", text );
    query.bindValue( ":l", level );

    if( query.exec() == false )  {
        return -1;
    }

    return query.lastInsertId().toInt();
}

Database::~Database()
{
    this->disconnect();
}
