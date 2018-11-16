#include "procesador.h"

Procesador * Procesador::instancia = NULL;

Procesador::Procesador( QObject * parent ) : QObject( parent ),
                                             max_features( 500 ),
                                             goodMatchPercent( 0.15f ),
                                             tipoCarnet( NONE ),
                                             ocr( new tesseract::TessBaseAPI )
{


    // Initialize tesseract to use Spanish (spa) and the LSTM OCR engine.
    ocr->Init( NULL, "spa", tesseract::OEM_LSTM_ONLY );

    // Set Page segmentation mode to PSM_AUTO (3)
    ocr->SetPageSegMode( tesseract::PSM_AUTO );

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

/**
 * @brief Procesador::configurarImageAlignment
 *
 * Se usa para configurar los parametros para la alineacion de los carnets y su imagen de referencia para features.
 *
 * Se debe llamar este metodo por cada tipo de carnet, para que pueda alinear a cada uno
 */
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
void Procesador::alinear( const cv::Mat & imParaAlinear, cv::Mat & imAlineada, TipoCarnet tipoCarnet )  {

    // Convert images to grayscale
    cv::Mat im1Gray, im2Gray;
    cv::cvtColor( imParaAlinear, im1Gray, CV_RGB2GRAY );

    if ( tipoCarnet == DNI )  {
        cv::cvtColor( this->imReferenciaDNI, im2Gray, CV_RGB2GRAY );
    }
    else if ( tipoCarnet == LICENCIA )  {
        cv::cvtColor( this->imReferenciaLicencia, im2Gray, CV_RGB2GRAY );
    }
    else if ( tipoCarnet == VERDE )  {
        cv::cvtColor( this->imReferenciaVerde, im2Gray, CV_RGB2GRAY );
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
 * @brief Procesador::extraerInfo Devuelve la info en este orden:
 * DNI - Apellido - Nombre - Domicilio - Fecha de nac. - Fecha de otorg. - Fecha de venc. - Clase
 * Este metodo guarda en la carpeta /imagenes/registros una imagen con la imParaProcesar con rectangulos rojos
 * de donde se extrae la informacion (incluida la foto y la firma).
 *
 * La imagen debe estar previamente alineada
 *
 * @return
 */
QStringList Procesador::extraerInfo( const cv::Mat & imParaProcesar,
                                     cv::Mat & imConRectangulos,
                                     Procesador::TipoCarnet tipoCarnet )
{
    QStringList textoExtraido;
    int sizeBordeRectangulo = 3;

    if ( tipoCarnet == DNI )  {


    }
    else if ( tipoCarnet == LICENCIA )  {

        imConRectangulos = imParaProcesar.clone();

        cv::Mat imDni = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.328652, imParaProcesar.rows * 0.213514 ),
                              cv::Point( imParaProcesar.cols * 0.573034, imParaProcesar.rows * 0.265766 ) ) );

        // Le damos la imagen para identificar texto y ejecuta el OCR con GetUTF8Text
        ocr->SetImage( imDni.data, imDni.cols, imDni.rows, 3, imDni.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        // Dibuja el rectangulo para registrar la region de la cual fue extraida la informacion
        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.328652, imParaProcesar.rows * 0.213514 ),
                       cv::Point( imParaProcesar.cols * 0.573034, imParaProcesar.rows * 0.265766 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imApellido = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.328652, imParaProcesar.rows * 0.313514 ),
                              cv::Point( imParaProcesar.cols * 0.814607, imParaProcesar.rows * 0.369369 ) ) );

        ocr->SetImage( imApellido.data, imApellido.cols, imApellido.rows, 3, imApellido.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.328652, imParaProcesar.rows * 0.313514 ),
                       cv::Point( imParaProcesar.cols * 0.814607, imParaProcesar.rows * 0.369369 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imNombre = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.328652, imParaProcesar.rows * 0.412613 ),
                              cv::Point( imParaProcesar.cols * 0.712360, imParaProcesar.rows * 0.470270 ) ) );

        ocr->SetImage( imNombre.data, imNombre.cols, imNombre.rows, 3, imNombre.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.328652, imParaProcesar.rows * 0.412613 ),
                       cv::Point( imParaProcesar.cols * 0.712360, imParaProcesar.rows * 0.470270 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );


        cv::Mat imDomicilio = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.328652, imParaProcesar.rows * 0.509009 ),
                              cv::Point( imParaProcesar.cols * 0.969101, imParaProcesar.rows * 0.552252 ) ) );

        ocr->SetImage( imDomicilio.data, imDomicilio.cols, imDomicilio.rows, 3, imDomicilio.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.328652, imParaProcesar.rows * 0.509009 ),
                       cv::Point( imParaProcesar.cols * 0.969101, imParaProcesar.rows * 0.552252 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );


        cv::Mat imFechaNac = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.328652, imParaProcesar.rows * 0.654955 ),
                              cv::Point( imParaProcesar.cols * 0.585955, imParaProcesar.rows * 0.712613 ) ) );

        ocr->SetImage( imFechaNac.data, imFechaNac.cols, imFechaNac.rows, 3, imFechaNac.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.328652, imParaProcesar.rows * 0.654955 ),
                       cv::Point( imParaProcesar.cols * 0.585955, imParaProcesar.rows * 0.712613 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );


        cv::Mat imFechaOtorg = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.328652, imParaProcesar.rows * 0.757658 ),
                              cv::Point( imParaProcesar.cols * 0.599438, imParaProcesar.rows * 0.812613 ) ) );

        ocr->SetImage( imFechaOtorg.data, imFechaOtorg.cols, imFechaOtorg.rows, 3, imFechaOtorg.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.328652, imParaProcesar.rows * 0.757658 ),
                       cv::Point( imParaProcesar.cols * 0.599438, imParaProcesar.rows * 0.812613 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );


        cv::Mat imFechaVenc = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.657303, imParaProcesar.rows * 0.752252 ),
                              cv::Point( imParaProcesar.cols * 0.995506, imParaProcesar.rows * 0.815915 ) ) );

        ocr->SetImage( imFechaVenc.data, imFechaVenc.cols, imFechaVenc.rows, 3, imFechaVenc.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.657303, imParaProcesar.rows * 0.752252 ),
                       cv::Point( imParaProcesar.cols * 0.995506, imParaProcesar.rows * 0.815915 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );


//        cv::Mat imFoto = imParaProcesar(
//                    cv::Rect( cv::Point( imParaProcesar.cols * 0.013483, imParaProcesar.rows * 0.184685 ),
//                              cv::Point( imParaProcesar.cols * 0.325843, imParaProcesar.rows * 0.806306 ) ) );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.013483, imParaProcesar.rows * 0.184685 ),
                       cv::Point( imParaProcesar.cols * 0.318843, imParaProcesar.rows * 0.806306 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imClase = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.919663, imParaProcesar.rows * 0.215315 ),
                              cv::Point( imParaProcesar.cols * 0.994944, imParaProcesar.rows * 0.279279 ) ) );

        ocr->SetImage( imClase.data, imClase.cols, imClase.rows, 3, imClase.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.919663, imParaProcesar.rows * 0.215315 ),
                       cv::Point( imParaProcesar.cols * 0.994944, imParaProcesar.rows * 0.279279 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::imwrite( "../imagenes/registros/rectangulos.jpg", imConRectangulos );

    }
    else if ( tipoCarnet == VERDE )  {


    }
    else  {
        return QStringList();
    }

    return textoExtraido;
}

/**
 * @brief Procesador::extraerFoto Extra una imagen solo de la foto.
 * La imagen debe estar previamente alineada.
 * Guarda en la carpeta registros la foto.
 *
 * @return Retorna un QImage que sera la foto de la paersona
 */
void Procesador::extraerFoto( const cv::Mat &imParaProcesar, cv::Mat & imFotoPerfil, Procesador::TipoCarnet tipoCarnet )
{
    if ( tipoCarnet == DNI )  {


    }
    else if ( tipoCarnet == LICENCIA )  {

        cv::Mat imFoto = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.013483, imParaProcesar.rows * 0.184685 ),
                              cv::Point( imParaProcesar.cols * 0.318843, imParaProcesar.rows * 0.806306 ) ) );

        imFotoPerfil = imFoto.clone();

    }
    else if ( tipoCarnet == VERDE )  {


    }
}

QImage Procesador::extraerFirma(cv::Mat &imParaProcesar, Procesador::TipoCarnet tipoCarnet)
{
    return QImage();
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
  cv::imwrite( "../imagenes/registros/matches.jpg", imMatches );


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
