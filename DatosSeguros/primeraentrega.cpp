#include "primeraentrega.h"
#include "ui_primeraentrega.h"

#include "procesador.h"
#include <QDebug>
#include <QFileDialog>

PrimeraEntrega::PrimeraEntrega(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PrimeraEntrega)
{
    ui->setupUi(this);

    connect( ui->pbElegirFoto, SIGNAL( pressed() ), this, SLOT( slot_elegirFoto() ) );
    connect( ui->pbProcesar, SIGNAL( pressed() ), this, SLOT( slot_procesar() ) );

    ui->visorFotoPerfil->setFixedSize( 150, 200 );
    ui->gbInfo->setFixedWidth( 300 );
}

PrimeraEntrega::~PrimeraEntrega()
{
    delete ui;
}

void PrimeraEntrega::resizeEvent(QResizeEvent *e)
{
    qDebug() << e->size();
}

void PrimeraEntrega::slot_procesar()
{
    cv::Mat imParaAlinear = ui->visor->getFrame();

    cv::Mat imAlineada;

    Procesador::getInstancia()->alinear( imParaAlinear, imAlineada, Procesador::LICENCIA );

    cv::Mat imConRectangulos;

    QStringList datosExtraidos = Procesador::getInstancia()->extraerInfo( imAlineada, imConRectangulos, Procesador::LICENCIA );

    ui->visor->setMat( imConRectangulos );

    ui->leDni->setText( datosExtraidos.at( 0 ) );
    ui->leApellido->setText( datosExtraidos.at( 1 ) );
    ui->leNombre->setText( datosExtraidos.at( 2 ) );
    ui->leNacimiento->setText( datosExtraidos.at( 4 ) );
    ui->leOtorgamiento->setText( datosExtraidos.at( 5 ) );
    ui->leVencimiento->setText( datosExtraidos.at( 6 ) );

    cv::Mat imFotoPerfil;

    Procesador::getInstancia()->extraerFoto( imAlineada, imFotoPerfil, Procesador::LICENCIA );

    ui->visorFotoPerfil->setMat( imFotoPerfil );

}

void PrimeraEntrega::slot_elegirFoto()
{
    QString archivo;
//    archivo = QFileDialog::getOpenFileName( this, "Abrir imagen", "/home/cosimani", "Images (*.png *.xpm *.jpg)");
//    ui->visor->setImagenDisco( archivo );

    std::string imFilename;

    if ( archivo.isEmpty() )
        imFilename = "../imagenes/LicenciaFoto5.jpg";
    else
        imFilename = archivo.toStdString();

    cv::Mat imParaProcesar = cv::imread( imFilename );

    cv::imwrite( "../imagenes/registros/imParaProcesar.jpg", imParaProcesar );

    ui->visor->setMat( imParaProcesar );

}
