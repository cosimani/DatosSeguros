#ifndef PRINCIPAL_H
#define PRINCIPAL_H

#include <QWidget>
#include <QResizeEvent>

#include <leptonica/allheaders.h>
#include <tesseract/baseapi.h>

#include <string>
#include <iostream>

#include <opencv/highgui.h>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>



//using namespace std;  // Ojo con esto porque tesseract tiene cosas iguales a la biblioteca de C++. Usar std::

namespace Ui {
class Principal;
}

class Principal : public QWidget
{
    Q_OBJECT

public:
    explicit Principal( QWidget * parent = 0 );
    ~Principal();

protected:
    void keyPressEvent( QKeyEvent * event );
    void resizeEvent( QResizeEvent * );

private:
    Ui::Principal *ui;

    tesseract::TessBaseAPI * ocr;

    cv::Mat frame;

    void alignImages( cv::Mat & im1, cv::Mat & im2, cv::Mat & im1Reg, cv::Mat & h );

    void alinear();

private slots:
    void slot_encenderCamara();
    void slot_abrirDelDisco();
    void slot_procesarTesseract();
    void slot_alinear();
};

#endif // PRINCIPAL_H