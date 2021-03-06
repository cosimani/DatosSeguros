#ifndef CONFIG_H
#define CONFIG_H

#define CONFIG_DIR "/../config/"
#define CONFIG_FILE_EXT ".ini"
#define CONFIG_FILE_COMMENTS "#"
#define CONFIG_FILE_ASSIGNMENT ":="

#include <QObject>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QVector>

#include "logger.h"

using namespace std;

class Config : public QObject
{
    Q_OBJECT

private:

    static Config *instance;
    static QVector<QStringList> *strings;
    explicit Config(QObject *parent=NULL);
    void init();

    bool ok;

public:

    static Config *getInstance();

    QString getString( QString key );
    void setString( QString key, QString value );

    bool isOk();

    string getStdString(QString key);
    const char *getCharString(QString key);

    ~Config();

};

#endif // CONFIG_H
