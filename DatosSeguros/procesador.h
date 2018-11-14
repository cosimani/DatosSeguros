#ifndef PROCESADOR_H
#define PROCESADOR_H

#include <QObject>

#include <string>
#include <iostream>

#include <opencv/highgui.h>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

#include <leptonica/allheaders.h>
#include <tesseract/baseapi.h>



class Procesador : public QObject
{
    Q_OBJECT


public:
    enum TipoCarnet { NONE, DNI, LICENCIA, VERDE };
    static Procesador * getInstancia();
    ~Procesador();

    TipoCarnet queEs( cv::Mat & imParaPreguntarQueEs );

    void configurarImageAlignment( TipoCarnet tipoCarnet = NONE,
                                   QString archivoReferencia = "",
                                   int max_features = 500,
                                   float goodMatchPercent = 0.15f
                                  );

    void alignImages( cv::Mat & im1, cv::Mat & im2, cv::Mat & im1Reg, cv::Mat & h );

    void alinear(cv::Mat &imParaAlinear, cv::Mat &imAlineada, TipoCarnet tipoCarnet = NONE);

private:
    static Procesador * instancia;
    explicit Procesador( QObject * parent = nullptr );

    int max_features;
    float goodMatchPercent;

    TipoCarnet tipoCarnet;

    cv::Mat imReferenciaDNI;
    cv::Mat imReferenciaLicencia;
    cv::Mat imReferenciaVerde;

signals:

public slots:
};

#endif // PROCESADOR_H
