#include "principal.h"
#include "ui_principal.h"
#include <QFileDialog>
#include <QDebug>

#include "procesador.h"

Principal::Principal(QWidget *parent) : QWidget(parent),
                                        ui(new Ui::Principal)

{
    ui->setupUi(this);




    ocr = new tesseract::TessBaseAPI();

    // Initialize tesseract to use Spanish (spa) and the LSTM OCR engine.
    ocr->Init( NULL, "spa", tesseract::OEM_LSTM_ONLY );

    // Set Page segmentation mode to PSM_AUTO (3)
    ocr->SetPageSegMode( tesseract::PSM_AUTO );

    connect( ui->pbEncender, SIGNAL( pressed() ), this, SLOT( slot_encenderCamara() ) );
    connect( ui->pbAbrirDelDisco, SIGNAL( pressed() ), this, SLOT( slot_abrirDelDisco() ) );
    connect( ui->pbTesseract, SIGNAL( pressed() ), this, SLOT( slot_procesarTesseract() ) );
    connect( ui->pbAlinear, SIGNAL( pressed() ), this, SLOT( slot_alinear() ) );



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

    //    // Open input image using OpenCV
    //    cv::Mat im = cv::imread( "../imagenes/texto1.jpg", IMREAD_COLOR );

    //    // Set image data
    //    ocr->SetImage( im.data, im.cols, im.rows, 3, im.step );

    //    // Run Tesseract OCR on image
    //    std::string outText = std::string( ocr->GetUTF8Text() );

    //    qDebug() << QString( outText.c_str() );

//    frame = ui->visor->getFrame();

//    // Set image data
//    ocr->SetImage( frame.data, frame.cols, frame.rows, 3, frame.step );

//    // Run Tesseract OCR on image
//    std::string outText = std::string( ocr->GetUTF8Text() );

//    QString texto( outText.c_str() );

//    ui->te->setText( texto );
}

void Principal::slot_abrirDelDisco()
{
//    QString archivo = QFileDialog::getOpenFileName( this, "Abrir imagen", "../", "Images (*.png *.xpm *.jpg)");
//    ui->visor->setImagenDisco( archivo );

    ui->visor->setImagenDisco( "../imagenes/texto1.jpg" );
}

void Principal::slot_procesarTesseract()
{
    frame = ui->visor->getFrame();

    // Set image data
    ocr->SetImage( frame.data, frame.cols, frame.rows, 3, frame.step );

    // Run Tesseract OCR on image
    std::string outText = std::string( ocr->GetUTF8Text() );

    QString texto( outText.c_str() );

    ui->te->setText( texto );
}

void Principal::slot_alinear()
{
    // Read reference image
    std::string refFilename( "../imagenes/referencias/LicenciaFrente.jpg" );
    qDebug() << "Reading reference image : " << QString( refFilename.c_str() );
    cv::Mat imReference = cv::imread( refFilename );


    // Read image to be aligned
    std::string imFilename( "../imagenes/LicenciaFoto4.jpg" );
    qDebug() << "Reading image to align : " << QString( imFilename.c_str() );
    cv::Mat im = cv::imread( imFilename );


    // Registered image will be resotred in imReg.
    // The estimated homography will be stored in h.
    cv::Mat imReg, h;

    // Align images
    qDebug() << "Aligning images ...";
    Procesador::getInstancia()->alignImages( im, imReference, imReg, h );
//    this->alignImages( im, imReference, imReg, h );

    // Write aligned image to disk.
    std::string outFilename( "../imagenes/LicenciaAligned.jpg" );
    qDebug() << "Saving aligned image : " << QString( outFilename.c_str() );
    cv::imwrite( outFilename, imReg );

    // Print estimated homography
    std::cout << "Estimated homography : \n" << h;

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
        ui->te->setText( "Se esta alineando" );
        break;


    default:;
    }
}

void Principal::resizeEvent(QResizeEvent *)
{
    ui->te->setMaximumWidth( this->width() / 2 );
}


const int MAX_FEATURES = 500;
const float GOOD_MATCH_PERCENT = 0.15f;


void Principal::alignImages( cv::Mat & im1, cv::Mat & im2, cv::Mat & im1Reg, cv::Mat & h )

{

  // Convert images to grayscale
  cv::Mat im1Gray, im2Gray;
  cv::cvtColor( im1, im1Gray, CV_BGR2GRAY );
  cv::cvtColor( im2, im2Gray, CV_BGR2GRAY );

  // Variables to store keypoints and descriptors
  std::vector< cv::KeyPoint > keypoints1, keypoints2;
  cv::Mat descriptors1, descriptors2;

  // Detect ORB features and compute descriptors.
  Ptr< cv::Feature2D > orb = cv::ORB::create( MAX_FEATURES );
  orb->detectAndCompute(im1Gray, Mat(), keypoints1, descriptors1);
  orb->detectAndCompute(im2Gray, Mat(), keypoints2, descriptors2);

  // Match features.
  std::vector<DMatch> matches;
  Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create( "BruteForce-Hamming" );
  matcher->match(descriptors1, descriptors2, matches, Mat());

  // Sort matches by score
  std::sort(matches.begin(), matches.end());

  // Remove not so good matches
  const int numGoodMatches = matches.size() * GOOD_MATCH_PERCENT;
  matches.erase(matches.begin()+numGoodMatches, matches.end());


  // Draw top matches
  Mat imMatches;
  drawMatches(im1, keypoints1, im2, keypoints2, matches, imMatches);
  imwrite("matches.jpg", imMatches);


  // Extract location of good matches
  std::vector<Point2f> points1, points2;

  for( size_t i = 0; i < matches.size(); i++ )
  {
    points1.push_back( keypoints1[ matches[i].queryIdx ].pt );
    points2.push_back( keypoints2[ matches[i].trainIdx ].pt );
  }

  // Find homography
  h = findHomography( points1, points2, RANSAC );

  // Use homography to warp image
  warpPerspective(im1, im1Reg, h, im2.size());

}

void Principal::alinear()  {

    // Si la camara esta activa, procesar su imagen, sino que levante del disco
    if ( ui->visor->getCamaraActiva() )  {

        cv::Mat imAlineada;

        cv::Mat imParaAlinear = ui->visor->getFrame();

        cv::cvtColor( imParaAlinear, imParaAlinear, CV_BGR2RGB );

        cv::imwrite( "../imagenes/matches.jpg", imParaAlinear );

        Procesador::getInstancia()->alinear( imParaAlinear, imAlineada, Procesador::LICENCIA );

        ui->visor->setMat( imAlineada );
    }
    else  {
        std::string imFilename( "../imagenes/LicenciaFoto4.jpg" );
        qDebug() << "Reading image to align : " << QString( imFilename.c_str() );
        cv::Mat imParaAlinear = cv::imread( imFilename );


        cv::Mat imAlineada;

        Procesador::getInstancia()->alinear( imParaAlinear, imAlineada, Procesador::LICENCIA );

        ui->visor->setMat( imAlineada );
    }
}

