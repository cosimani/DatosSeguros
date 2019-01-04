#ifndef TESTEADOR_H
#define TESTEADOR_H

#include <QObject>
#include <QList>
#include <QVector>
#include <QByteArray>
#include <QStringList>
#include <QDir>
#include <QImage>

#include <opencv/highgui.h>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

#include <QZXing.h>

#include "conexion.h"

class Testeador : public QObject  {
    Q_OBJECT

public:
    explicit Testeador( QObject * parent = 0 );
    ~Testeador();

    void procesar(QString directorio );
    void procesar2(QString directorio );
    void procesar3(QString directorio);
    void procesar4( QString directorio );
    void procesar5( QString directorio );
    void procesarDniDorso( QString directorio );


    cv::Vec3f calcParams(cv::Point2f p1, cv::Point2f p2);
    cv::Point findIntersection(cv::Vec3f params1, cv::Vec3f params2);
    std::vector<cv::Point> getQuadrilateral(cv::Mat &grayscale, cv::Mat &output);
private:
    QImage im;
    QZXing * decoder;


};

#endif //TESTEADOR_H
