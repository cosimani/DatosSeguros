#ifndef DATABASE_HPP
#define DATABASE_HPP

#define DATABASE_DRIVER "database_driver"
#define DATABASE_URI "database_uri"

#define LOGIN_QUERY "login_query"
#define SESSION_QUERY "session_query"
#define PLATE_DETECTION_QUERY "plate_detection_query"
#define CANDIDATE_QUERY "candidate_query"
#define LOG_QUERY "log_query"

#define LINF 1
#define LWAR 2
#define LERR 3

#include <QObject>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QVector>
#include <QStringList>
#include <QSqlRecord>

#include <stdexcept>

#include "logger.h"
#include "config.h"

class Database : public QObject
{
    Q_OBJECT

private:

    static Database *instance;
    explicit Database(QObject *parent = NULL);

    QSqlDatabase database;

    QString getLoginQuery();
    QString getSessionQuery();
    QString getPlateDetectionQuery();
    QString getCandidateQuery();
    QString getLogQuery();

public:

    bool databaseOk;
    static Database *getInstance();
    ~Database();

    void connect();
    void disconnect();

    int login( QString user, QString password );
    int startSession( int adminId );
    int plateDetection(int sessionId);
    int nuevaPersona(QString dni, QString apellido, QString nombre, QString sexo);
    int log(QString text, int level);
    int actualizarPersona( QString dni, QString apellido, QString nombre, QString sexo );
    QVector< QStringList > todasLasPersonas();

    /**
     * @brief procesarJson este metodo analiza los datos que hay en este json y hace todo lo que tenga que hacer.
     * Si detecta una nueva persona, entonces crea una entrada nueva con clave dni, si ya existe, actualiza los datos. El json
     * que este metodo recibe ya debe estar correctos.
     * @param json
     * @return
     */
    bool procesarJson( QString json );
};

#endif // DATABASE_HPP
