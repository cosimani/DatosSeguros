#include "principal.h"
#include "ui_principal.h"
#include <QFileDialog>
#include <QDebug>

#include "procesador.h"

Principal::Principal(QWidget *parent) : QWidget(parent),
                                        ui(new Ui::Principal)

{
    ui->setupUi(this);



    connect( ui->pbEncender, SIGNAL( pressed() ), this, SLOT( slot_encenderCamara() ) );
    connect( ui->pbAbrirDelDisco, SIGNAL( pressed() ), this, SLOT( slot_abrirDelDisco() ) );



    Procesador::getInstancia()->configurarImageAlignment( Procesador::LICENCIA,
                                                          "../imagenes/referencias/LicenciaFrente.jpg" );

}

Principal::~Principal()
{
    delete ui;
}

void Principal::slot_encenderCamara()
{
    ui->visor->iniciarCamara( 20, 1 );
}

void Principal::slot_abrirDelDisco()
{
//    QString archivo = QFileDialog::getOpenFileName( this, "Abrir imagen", "../", "Images (*.png *.xpm *.jpg)");
//    ui->visor->setImagenDisco( archivo );

    ui->visor->setImagenDisco( "../imagenes/texto1.jpg" );
}



void Principal::keyPressEvent( QKeyEvent *event )
{
    switch( event->key() )
    {
    case Qt::Key_Escape:
        qApp->quit();
        break;

    case Qt::Key_C:
        this->slot_encenderCamara();
        break;

    case Qt::Key_A:
        this->alinear();
        break;


    default:;
    }
}

void Principal::resizeEvent(QResizeEvent *)
{
    ui->te->setMaximumWidth( this->width() / 2 );
}



void Principal::alinear()  {

    // Si la camara esta activa, procesar su imagen, sino que levante del disco
    if ( ui->visor->getCamaraActiva() )  {

        cv::Mat imAlineada;

        cv::Mat imParaAlinear = ui->visor->getFrame();

        cv::cvtColor( imParaAlinear, imParaAlinear, CV_BGR2RGB );

        cv::imwrite( "../imagenes/matches.jpg", imParaAlinear );

        Procesador::getInstancia()->alinear( imParaAlinear, imAlineada, Procesador::LICENCIA );


    }
    else  {
//        std::string imFilename( "../imagenes/LicenciaFoto4.jpg" );
//        qDebug() << "Reading image to align : " << QString( imFilename.c_str() );
//        cv::Mat imParaAlinear = cv::imread( imFilename );

//        cv::Mat imAlineada;

//        Procesador::getInstancia()->alinear( imParaAlinear, imAlineada, Procesador::LICENCIA );

//        qDebug() << Procesador::getInstancia()->extraerInfo( imAlineada, Procesador::LICENCIA );



    }
}

