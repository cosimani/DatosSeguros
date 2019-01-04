#ifndef PROCESADOR_H
#define PROCESADOR_H

#include <QObject>
//#include <QImage>
#include <QDebug>

#include <string>
#include <iostream>

#include <opencv/highgui.h>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/features2d.hpp>

#include <opencv2/xfeatures2d.hpp>

#include <leptonica/allheaders.h>
#include <tesseract/baseapi.h>



class Procesador : public QObject
{
    Q_OBJECT


public:
    enum TipoCarnet { NONE, DNI, LICENCIA, VERDE, DNI_DORSO, LICENCIA_DORSO, VERDE_DORSO };
    static Procesador * getInstancia();
    ~Procesador();

    TipoCarnet queEs( cv::Mat & imParaPreguntarQueEs );

    void configurarImageAlignment( TipoCarnet tipoCarnet = NONE,
                                   QString archivoReferencia = "",
                                   int max_features = 500,
                                   float goodMatchPercent = 0.15f
                                  );

    void alignImages( cv::Mat & im1, cv::Mat & im2, cv::Mat & im1Reg, cv::Mat & h );

    bool alinearSurfFlann( const cv::Mat &imParaAlinear, cv::Mat &imAlineada, TipoCarnet tipoCarnet = NONE );

    void alinear( const cv::Mat &imParaAlinear, cv::Mat &imAlineada, TipoCarnet tipoCarnet = NONE );

    QStringList extraerInfo( const cv::Mat & imParaProcesar, cv::Mat & imConRectangulos, TipoCarnet tipoCarnet = NONE );

    void extraerFoto( const  cv::Mat &imParaProcesar, cv::Mat & imFotoPerfil, TipoCarnet tipoCarnet = NONE );

//    QImage extraerFirma( cv::Mat &imParaProcesar, TipoCarnet tipoCarnet = NONE );

    void alinearOtraOpcion(const cv::Mat &imParaAlinear, cv::Mat &imAlineada, TipoCarnet tipoCarnet);
    QString extraerTexto(const cv::Mat &imParaProcesar, Procesador::TipoCarnet tipoCarnet);

private:
    static Procesador * instancia;
    explicit Procesador( QObject * parent = nullptr );

    int max_features;
    float goodMatchPercent;

    TipoCarnet tipoCarnet;

    cv::Mat imReferenciaDNI;
    cv::Mat imReferenciaLicencia;
    cv::Mat imReferenciaVerde;
    cv::Mat imReferenciaDNI_dorso;
    cv::Mat imReferenciaLicencia_dorso;
    cv::Mat imReferenciaVerde_dorso;

    tesseract::TessBaseAPI * ocrLinea;
    tesseract::TessBaseAPI * ocrParrafo;

signals:

public slots:
};

#endif // PROCESADOR_H
