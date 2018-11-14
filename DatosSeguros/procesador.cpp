#include "procesador.h"

Procesador * Procesador::instancia = NULL;

Procesador::Procesador( QObject * parent ) : QObject( parent ),
                                             max_features( 500 ),
                                             goodMatchPercent( 0.15f ),
                                             tipoCarnet( NONE )
{
    this->configurarImageAlignment();
}

Procesador * Procesador::getInstancia()  {
    if( instancia == NULL )  {
        instancia = new Procesador;
    }
    return instancia;
}

Procesador::~Procesador()  {
    if( instancia != NULL )  {
        delete instancia;
    }
}

Procesador::TipoCarnet Procesador::queEs(cv::Mat &imParaPreguntarQueEs)
{
    return NONE;
}

void Procesador::configurarImageAlignment( TipoCarnet tipoCarnet,
                                           QString archivoReferencia,
                                           int max_features,
                                           float goodMatchPercent  )
{
    // Si entra a este if entocnes nada fue configurado
    if ( tipoCarnet == NONE )  {
        return;
    }

    this->max_features = max_features;
    this->goodMatchPercent = goodMatchPercent;

    std::string refFilename( archivoReferencia.toStdString().c_str() );
    cv::Mat imReference = cv::imread( refFilename );
    if ( imReference.empty() )  {
        tipoCarnet = NONE;
        return;
    }

    if ( tipoCarnet == DNI )  {
        this->imReferenciaDNI = imReference.clone();
    }
    else if ( tipoCarnet == LICENCIA )  {
        this->imReferenciaLicencia = imReference.clone();
    }
    else if ( tipoCarnet == VERDE )  {
        this->imReferenciaVerde = imReference.clone();
    }
    else  {
        tipoCarnet = NONE;
    }
}

/**
 * @brief Procesador::alignImages Alinea las imagenes en base a una imagen de muestra.
 * @param im1 Es la imagen que se desea alinear
 * @param im2 Es la imagen de referencia
 * @param im1Reg Es la imagen im1 alineada
 * @param h Es la matriz de transformacion
 */
void Procesador::alinear( cv::Mat & imParaAlinear, cv::Mat & imAlineada, TipoCarnet tipoCarnet )  {

    // Convert images to grayscale
    cv::Mat im1Gray, im2Gray;
    cv::cvtColor( imParaAlinear, im1Gray, CV_BGR2GRAY );

    if ( tipoCarnet == DNI )  {
        cv::cvtColor( this->imReferenciaDNI, im2Gray, CV_BGR2GRAY );
    }
    else if ( tipoCarnet == LICENCIA )  {
        cv::cvtColor( this->imReferenciaLicencia, im2Gray, CV_BGR2GRAY );
    }
    else if ( tipoCarnet == VERDE )  {
        cv::cvtColor( this->imReferenciaVerde, im2Gray, CV_BGR2GRAY );
    }
    else  {
        return;
    }

    // Variables to store keypoints and descriptors
    std::vector< cv::KeyPoint > keypoints1, keypoints2;
    cv::Mat descriptors1, descriptors2;

    // Detect ORB features and compute descriptors.
    cv::Ptr< cv::Feature2D > orb = cv::ORB::create( this->max_features );
    orb->detectAndCompute(im1Gray, cv::Mat(), keypoints1, descriptors1);
    orb->detectAndCompute(im2Gray, cv::Mat(), keypoints2, descriptors2);

    // Match features.
    std::vector< cv::DMatch > matches;
    cv::Ptr< cv::DescriptorMatcher > matcher = cv::DescriptorMatcher::create( "BruteForce-Hamming" );
    matcher->match( descriptors1, descriptors2, matches, cv::Mat() );

    // Sort matches by score
    std::sort( matches.begin(), matches.end() );

    // Remove not so good matches
    const int numGoodMatches = matches.size() * this->goodMatchPercent;
    matches.erase( matches.begin() + numGoodMatches, matches.end() );


    // Draw top matches
    cv::Mat imMatches;

    if ( tipoCarnet == DNI )  {
        cv::drawMatches( imParaAlinear, keypoints1, this->imReferenciaDNI, keypoints2, matches, imMatches );
    }
    else if ( tipoCarnet == LICENCIA )  {
        cv::drawMatches( imParaAlinear, keypoints1, this->imReferenciaLicencia, keypoints2, matches, imMatches );
    }
    else if ( tipoCarnet == VERDE )  {
        cv::drawMatches( imParaAlinear, keypoints1, this->imReferenciaVerde, keypoints2, matches, imMatches );
    }
    else  {
        return;
    }

    cv::imwrite( "../imagenes/matches.jpg", imMatches );

    // Extract location of good matches
    std::vector< cv::Point2f > points1, points2;

    for( size_t i = 0; i < matches.size(); i++ )
    {
    points1.push_back( keypoints1[ matches[i].queryIdx ].pt );
    points2.push_back( keypoints2[ matches[i].trainIdx ].pt );
    }

    // Find homography
    cv::Mat h = cv::findHomography( points1, points2, cv::RANSAC );

    if ( tipoCarnet == DNI )  {
        cv::warpPerspective( imParaAlinear, imAlineada, h, this->imReferenciaDNI.size() );
    }
    else if ( tipoCarnet == LICENCIA )  {
        cv::warpPerspective( imParaAlinear, imAlineada, h, this->imReferenciaLicencia.size() );
    }
    else if ( tipoCarnet == VERDE )  {
        cv::warpPerspective( imParaAlinear, imAlineada, h, this->imReferenciaVerde.size() );
    }
    else  {
        return;
    }

}

/**
 * @brief Procesador::alignImages Alinea las imagenes en base a una imagen de muestra.
 * @param im1 Es la imagen que se desea alinear
 * @param im2 Es la imagen de referencia
 * @param im1Reg Es la imagen im1 alineada
 * @param h Es la matriz de transformacion
 */
void Procesador::alignImages( cv::Mat & im1, cv::Mat & im2, cv::Mat & im1Reg, cv::Mat & h )  {

  // Convert images to grayscale
  cv::Mat im1Gray, im2Gray;
  cv::cvtColor( im1, im1Gray, CV_BGR2GRAY );
  cv::cvtColor( im2, im2Gray, CV_BGR2GRAY );

  // Variables to store keypoints and descriptors
  std::vector< cv::KeyPoint > keypoints1, keypoints2;
  cv::Mat descriptors1, descriptors2;

  // Detect ORB features and compute descriptors.
  cv::Ptr< cv::Feature2D > orb = cv::ORB::create( this->max_features );
  orb->detectAndCompute(im1Gray, cv::Mat(), keypoints1, descriptors1);
  orb->detectAndCompute(im2Gray, cv::Mat(), keypoints2, descriptors2);

  // Match features.
  std::vector< cv::DMatch > matches;
  cv::Ptr< cv::DescriptorMatcher > matcher = cv::DescriptorMatcher::create( "BruteForce-Hamming" );
  matcher->match( descriptors1, descriptors2, matches, cv::Mat() );

  // Sort matches by score
  std::sort( matches.begin(), matches.end() );

  // Remove not so good matches
  const int numGoodMatches = matches.size() * this->goodMatchPercent;
  matches.erase( matches.begin() + numGoodMatches, matches.end() );


  // Draw top matches
  cv::Mat imMatches;
  cv::drawMatches( im1, keypoints1, im2, keypoints2, matches, imMatches );
  cv::imwrite( "../imagenes/matches.jpg", imMatches );


  // Extract location of good matches
  std::vector< cv::Point2f > points1, points2;

  for( size_t i = 0; i < matches.size(); i++ )
  {
    points1.push_back( keypoints1[ matches[i].queryIdx ].pt );
    points2.push_back( keypoints2[ matches[i].trainIdx ].pt );
  }

  // Find homography
  h = cv::findHomography( points1, points2, cv::RANSAC );

  // Use homography to warp image
  cv::warpPerspective( im1, im1Reg, h, im2.size() );

}
