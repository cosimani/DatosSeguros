#include "visor.h"
#include <QApplication>

#include <QMessageBox>
#include <QFileInfo>
#include <QDebug>


bool Visor::getCamaraActiva() const
{
    return camaraActiva;
}

Visor::Visor( QWidget *parent ) : QLabel( parent ),
    videoCapture ( new cv::VideoCapture ),
                                  sceneTimer ( new QTimer( this ) ),
                                  camaraActiva( false )
{

    connect( sceneTimer, SIGNAL( timeout() ), SLOT( slot_procesar() ) );


}

Visor::~Visor()
{
    videoCapture->release();
}

const Mat &Visor::getFrame()  {
    return frame;
}

/**
 * @brief Visor::iniciarCamara Inicia la camara con esos milisegundos y la camara elegida
 * @param msTimer
 * @param nroCamara
 */
void Visor::iniciarCamara( int msTimer, int nroCamara )  {
    sceneTimer->start( msTimer );
    videoCapture->open( nroCamara );
    this->camaraActiva = true;
}

void Visor::setMat(cv::Mat im)
{
    sceneTimer->stop();
    this->camaraActiva = false;

    if ( videoCapture->isOpened() )
        videoCapture->release();

    if ( ! im.empty() )  {
        cvtColor( im, im, CV_BGR2RGB );
        frame = im.clone();
    }

    this->slot_procesar();
}

void Visor::setImagenDisco( QString archivo )
{
    sceneTimer->stop();
    this->camaraActiva = false;

    if ( videoCapture->isOpened() )
        videoCapture->release();

    if ( QFile( archivo ).exists() )  {
        cv::Mat im = cv::imread( archivo.toStdString().c_str(), IMREAD_COLOR );

        if ( ! im.empty() )  {
            frame = im.clone();
        }
    }

    this->slot_procesar();
}

void Visor::slot_procesar()  {

    if ( camaraActiva )  {
        videoCapture->operator >>( frame );

        cvtColor( frame, frame, CV_BGR2RGB );

        // Procesar aqui

        QPixmap pixmap = QPixmap::fromImage( QImage( frame.data,
                                                     frame.cols,
                                                     frame.rows,
                                                     frame.step,
                                                     QImage::Format_RGB888
                                                   ).scaled( this->width(), this->height() )
                                           );
        this->setPixmap( pixmap );
    }
    else  {  // Aca coloca la ultima imagen levantada del disco o negro si no hay nada

        if ( frame.empty() )  {  // Imagen en negro
            this->setPixmap( QPixmap( this->width(), this->height() ) );
        }
        else  {  // Imagen del disco
            QPixmap pixmap = QPixmap::fromImage( QImage( frame.data,
                                                         frame.cols,
                                                         frame.rows,
                                                         frame.step,
                                                         QImage::Format_RGB888
                                                       ).scaled( this->width(), this->height() )
                                               );
            this->setPixmap( pixmap );
        }
    }
}









