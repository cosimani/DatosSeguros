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
    Q_UNUSED( imParaPreguntarQueEs );
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
    else if ( tipoCarnet == DNI_DORSO )  {
        this->imReferenciaDNI_dorso = imReference.clone();
    }
    else if ( tipoCarnet == LICENCIA_DORSO )  {
        this->imReferenciaLicencia_dorso = imReference.clone();
    }
    else if ( tipoCarnet == VERDE_DORSO )  {
        this->imReferenciaVerde_dorso = imReference.clone();
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
    else if ( tipoCarnet == DNI_DORSO )  {
        cv::cvtColor( this->imReferenciaDNI_dorso, im2Gray, CV_RGB2GRAY );
    }
    else if ( tipoCarnet == LICENCIA_DORSO )  {
        cv::cvtColor( this->imReferenciaLicencia_dorso, im2Gray, CV_RGB2GRAY );
    }
    else if ( tipoCarnet == VERDE_DORSO )  {
        cv::cvtColor( this->imReferenciaVerde_dorso, im2Gray, CV_RGB2GRAY );
    }
    else  {
        return;
    }

    cv::Ptr< cv::Feature2D > f2d = cv::xfeatures2d::SIFT::create();
    //cv::Ptr<Feature2D> f2d = xfeatures2d::SURF::create();
    //cv::Ptr<Feature2D> f2d = ORB::create();

    // Variables to store keypoints and descriptors
    std::vector< cv::KeyPoint > keypoints1, keypoints2;
    cv::Mat descriptors1, descriptors2;

    // Detect ORB features and compute descriptors.
//    cv::Ptr< cv::Feature2D > f2d = cv::ORB::create( this->max_features );
    f2d->detectAndCompute( im1Gray, cv::Mat(), keypoints1, descriptors1 );
    f2d->detectAndCompute( im2Gray, cv::Mat(), keypoints2, descriptors2 );

    // Match features.
//    std::vector< cv::DMatch > matches;
//    cv::Ptr< cv::DescriptorMatcher > matcher = cv::DescriptorMatcher::create( "BruteForce-Hamming" );
//    matcher->match( descriptors1, descriptors2, matches, cv::Mat() );

    cv::BFMatcher matcher;
    std::vector< cv::DMatch > matches;
    matcher.match( descriptors1, descriptors2, matches );

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
    else if ( tipoCarnet == DNI_DORSO )  {
        cv::drawMatches( imParaAlinear, keypoints1, this->imReferenciaDNI_dorso, keypoints2, matches, imMatches );
    }
    else if ( tipoCarnet == LICENCIA_DORSO )  {
        cv::drawMatches( imParaAlinear, keypoints1, this->imReferenciaLicencia_dorso, keypoints2, matches, imMatches );
    }
    else if ( tipoCarnet == VERDE_DORSO )  {
        cv::drawMatches( imParaAlinear, keypoints1, this->imReferenciaVerde_dorso, keypoints2, matches, imMatches );
    }
    else  {
        return;
    }

#ifdef EJECUTADO_EN_SERVER
    cv::imwrite( "imagenes/registros/matches.jpg", imMatches );
#else
    cv::imwrite( "../imagenes/registros/matches.jpg", imMatches );
#endif

    // Extract location of good matches
    std::vector< cv::Point2f > points1, points2;

    for( size_t i = 0; i < matches.size(); i++ )
    {
        points1.push_back( keypoints1[ matches[i].queryIdx ].pt );
        points2.push_back( keypoints2[ matches[i].trainIdx ].pt );
    }

    // Si hay pocos puntos, retornamos porque findHomography tiraria error
    if ( points1.size() < 4 || points2.size() < 4 )
        return;

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
    else if ( tipoCarnet == DNI_DORSO )  {
        cv::warpPerspective( imParaAlinear, imAlineada, h, this->imReferenciaDNI_dorso.size() );
    }
    else if ( tipoCarnet == LICENCIA_DORSO )  {
        cv::warpPerspective( imParaAlinear, imAlineada, h, this->imReferenciaLicencia_dorso.size() );
    }
    else if ( tipoCarnet == VERDE_DORSO )  {
        cv::warpPerspective( imParaAlinear, imAlineada, h, this->imReferenciaVerde_dorso.size() );
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

        imConRectangulos = imParaProcesar.clone();

        cv::Mat imDni = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.052147, imParaProcesar.rows * 0.902941 ),
                              cv::Point( imParaProcesar.cols * 0.292025, imParaProcesar.rows * 0.973529 ) ) );

        // Le damos la imagen para identificar texto y ejecuta el OCR con GetUTF8Text
        ocr->SetImage( imDni.data, imDni.cols, imDni.rows, 3, imDni.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        // Dibuja el rectangulo para registrar la region de la cual fue extraida la informacion
        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.052147, imParaProcesar.rows * 0.902941 ),
                       cv::Point( imParaProcesar.cols * 0.292025, imParaProcesar.rows * 0.973529 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imApellido = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.367485, imParaProcesar.rows * 0.229412 ),
                              cv::Point( imParaProcesar.cols * 0.560123, imParaProcesar.rows * 0.274510 ) ) );

        ocr->SetImage( imApellido.data, imApellido.cols, imApellido.rows, 3, imApellido.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.367485, imParaProcesar.rows * 0.229412 ),
                       cv::Point( imParaProcesar.cols * 0.560123, imParaProcesar.rows * 0.274510 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imNombre = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.365031, imParaProcesar.rows * 0.365686 ),
                              cv::Point( imParaProcesar.cols * 0.666258, imParaProcesar.rows * 0.416667 ) ) );

        ocr->SetImage( imNombre.data, imNombre.cols, imNombre.rows, 3, imNombre.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.365031, imParaProcesar.rows * 0.365686 ),
                       cv::Point( imParaProcesar.cols * 0.666258, imParaProcesar.rows * 0.416667 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );


        // Este es el dato "Sexo"
        cv::Mat imDomicilio = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.365031, imParaProcesar.rows * 0.506863 ),
                              cv::Point( imParaProcesar.cols * 0.401227, imParaProcesar.rows * 0.546078 ) ) );

        ocr->SetImage( imDomicilio.data, imDomicilio.cols, imDomicilio.rows, 3, imDomicilio.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.365031, imParaProcesar.rows * 0.506863 ),
                       cv::Point( imParaProcesar.cols * 0.401227, imParaProcesar.rows * 0.546078 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );


        cv::Mat imFechaNac = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.368098, imParaProcesar.rows * 0.599020 ),
                              cv::Point( imParaProcesar.cols * 0.621472, imParaProcesar.rows * 0.639216 ) ) );

        ocr->SetImage( imFechaNac.data, imFechaNac.cols, imFechaNac.rows, 3, imFechaNac.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.368098, imParaProcesar.rows * 0.599020 ),
                       cv::Point( imParaProcesar.cols * 0.621472, imParaProcesar.rows * 0.639216 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imFechaOtorg = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.367847, imParaProcesar.rows * 0.690196 ),
                              cv::Point( imParaProcesar.cols * 0.615951, imParaProcesar.rows * 0.731373 ) ) );

        ocr->SetImage( imFechaOtorg.data, imFechaOtorg.cols, imFechaOtorg.rows, 3, imFechaOtorg.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.367847, imParaProcesar.rows * 0.690196 ),
                       cv::Point( imParaProcesar.cols * 0.615951, imParaProcesar.rows * 0.731373 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imFechaVenc = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.367485, imParaProcesar.rows * 0.784314 ),
                              cv::Point( imParaProcesar.cols * 0.626380, imParaProcesar.rows * 0.824510 ) ) );

        ocr->SetImage( imFechaVenc.data, imFechaVenc.cols, imFechaVenc.rows, 3, imFechaVenc.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.367485, imParaProcesar.rows * 0.784314 ),
                       cv::Point( imParaProcesar.cols * 0.626380, imParaProcesar.rows * 0.824510 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );


//        cv::Mat imFoto = imParaProcesar(
//                    cv::Rect( cv::Point( imParaProcesar.cols * 0.054601, imParaProcesar.rows * 0.223529 ),
//                              cv::Point( imParaProcesar.cols * 0.352761, imParaProcesar.rows * 0.827451 ) ) );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.054601, imParaProcesar.rows * 0.223529 ),
                       cv::Point( imParaProcesar.cols * 0.352761, imParaProcesar.rows * 0.827451 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        // Este es el dato "Ejemplar"
        cv::Mat imClase = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.742331, imParaProcesar.rows * 0.509804 ),
                              cv::Point( imParaProcesar.cols * 0.776687, imParaProcesar.rows * 0.545098 ) ) );

        ocr->SetImage( imClase.data, imClase.cols, imClase.rows, 3, imClase.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.742331, imParaProcesar.rows * 0.509804 ),
                       cv::Point( imParaProcesar.cols * 0.776687, imParaProcesar.rows * 0.545098 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );


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

    }
    else if ( tipoCarnet == VERDE )  {

        // Extrae:   Dominio / Modelo / Tipo / Uso / Chasis / Motor / Vencimiento / Marca

        imConRectangulos = imParaProcesar.clone();

        cv::Mat imDominio = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.314206, imParaProcesar.rows * 0.300778 ),
                              cv::Point( imParaProcesar.cols * 0.485237, imParaProcesar.rows * 0.336214 ) ) );

        // Le damos la imagen para identificar texto y ejecuta el OCR con GetUTF8Text
        ocr->SetImage( imDominio.data, imDominio.cols, imDominio.rows, 3, imDominio.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        // Dibuja el rectangulo para registrar la region de la cual fue extraida la informacion
        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.314206, imParaProcesar.rows * 0.300778 ),
                       cv::Point( imParaProcesar.cols * 0.485237, imParaProcesar.rows * 0.336214 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imModelo = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.310306, imParaProcesar.rows * 0.391530 ),
                              cv::Point( imParaProcesar.cols * 0.523120, imParaProcesar.rows * 0.432152 ) ) );

        ocr->SetImage( imModelo.data, imModelo.cols, imModelo.rows, 3, imModelo.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.310306, imParaProcesar.rows * 0.391530 ),
                       cv::Point( imParaProcesar.cols * 0.523120, imParaProcesar.rows * 0.432152 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imTipo = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.261281, imParaProcesar.rows * 0.439931 ),
                              cv::Point( imParaProcesar.cols * 0.509749, imParaProcesar.rows * 0.477960 ) ) );

        ocr->SetImage( imTipo.data, imTipo.cols, imTipo.rows, 3, imTipo.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.261281, imParaProcesar.rows * 0.439931 ),
                       cv::Point( imParaProcesar.cols * 0.509749, imParaProcesar.rows * 0.477960 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imUso = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.257939, imParaProcesar.rows * 0.490061 ),
                              cv::Point( imParaProcesar.cols * 0.389972, imParaProcesar.rows * 0.526361 ) ) );

        ocr->SetImage( imUso.data, imUso.cols, imUso.rows, 3, imUso.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.257939, imParaProcesar.rows * 0.490061 ),
                       cv::Point( imParaProcesar.cols * 0.389972, imParaProcesar.rows * 0.526361 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );


        cv::Mat imChasis = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.304178, imParaProcesar.rows * 0.540190 ),
                              cv::Point( imParaProcesar.cols * 0.585515, imParaProcesar.rows * 0.574762 ) ) );

        ocr->SetImage( imChasis.data, imChasis.cols, imChasis.rows, 3, imChasis.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.304178, imParaProcesar.rows * 0.540190 ),
                       cv::Point( imParaProcesar.cols * 0.585515, imParaProcesar.rows * 0.574762 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imMotor = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.300279, imParaProcesar.rows * 0.588591 ),
                              cv::Point( imParaProcesar.cols * 0.537604, imParaProcesar.rows * 0.625756 ) ) );

        ocr->SetImage( imMotor.data, imMotor.cols, imMotor.rows, 3, imMotor.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.300279, imParaProcesar.rows * 0.588591 ),
                       cv::Point( imParaProcesar.cols * 0.537604, imParaProcesar.rows * 0.625756 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imVencimiento = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.293593, imParaProcesar.rows * 0.634399 ),
                              cv::Point( imParaProcesar.cols * 0.457382, imParaProcesar.rows * 0.670700 ) ) );

        ocr->SetImage( imVencimiento.data, imVencimiento.cols, imVencimiento.rows, 3, imVencimiento.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.293593, imParaProcesar.rows * 0.634399 ),
                       cv::Point( imParaProcesar.cols * 0.457382, imParaProcesar.rows * 0.670700 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imMarca = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.289694, imParaProcesar.rows * 0.344857 ),
                              cv::Point( imParaProcesar.cols * 0.448468, imParaProcesar.rows * 0.382022 ) ) );

        ocr->SetImage( imMarca.data, imMarca.cols, imMarca.rows, 3, imMarca.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.289694, imParaProcesar.rows * 0.344857 ),
                       cv::Point( imParaProcesar.cols * 0.448468, imParaProcesar.rows * 0.382022 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );
    }
    else if ( tipoCarnet == DNI_DORSO )  {
        // Extrae:   Domicilio / Nacimiento / Cuil / Linea1 / Linea2 / Linea3

        imConRectangulos = imParaProcesar.clone();

        cv::Mat imDomicilio = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.158114, imParaProcesar.rows * 0.095344 ),
                              cv::Point( imParaProcesar.cols * 0.689320, imParaProcesar.rows * 0.136363 ) ) );

        // Le damos la imagen para identificar texto y ejecuta el OCR con GetUTF8Text
        ocr->SetImage( imDomicilio.data, imDomicilio.cols, imDomicilio.rows, 3, imDomicilio.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        // Dibuja el rectangulo para registrar la region de la cual fue extraida la informacion
        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.158114, imParaProcesar.rows * 0.095344 ),
                       cv::Point( imParaProcesar.cols * 0.689320, imParaProcesar.rows * 0.136363 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imNacimiento = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.275312, imParaProcesar.rows * 0.177384 ),
                              cv::Point( imParaProcesar.cols * 0.537448, imParaProcesar.rows * 0.212860 ) ) );

        ocr->SetImage( imNacimiento.data, imNacimiento.cols, imNacimiento.rows, 3, imNacimiento.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.275312, imParaProcesar.rows * 0.177384 ),
                       cv::Point( imParaProcesar.cols * 0.537448, imParaProcesar.rows * 0.212860 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imCuil = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.112344, imParaProcesar.rows * 0.587583 ),
                              cv::Point( imParaProcesar.cols * 0.312067, imParaProcesar.rows * 0.621951 ) ) );

        ocr->SetImage( imCuil.data, imCuil.cols, imCuil.rows, 3, imCuil.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.112344, imParaProcesar.rows * 0.587583 ),
                       cv::Point( imParaProcesar.cols * 0.312067, imParaProcesar.rows * 0.621951 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imLinea1 = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.047157, imParaProcesar.rows * 0.743902 ),
                              cv::Point( imParaProcesar.cols * 0.953537, imParaProcesar.rows * 0.801552 ) ) );

        ocr->SetImage( imLinea1.data, imLinea1.cols, imLinea1.rows, 3, imLinea1.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.047157, imParaProcesar.rows * 0.743902 ),
                       cv::Point( imParaProcesar.cols * 0.953537, imParaProcesar.rows * 0.801552 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );


        cv::Mat imLinea2 = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.047157, imParaProcesar.rows * 0.818182 ),
                              cv::Point( imParaProcesar.cols * 0.953537, imParaProcesar.rows * 0.885809 ) ) );

        ocr->SetImage( imLinea2.data, imLinea2.cols, imLinea2.rows, 3, imLinea2.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.047157, imParaProcesar.rows * 0.818182 ),
                       cv::Point( imParaProcesar.cols * 0.953537, imParaProcesar.rows * 0.885809 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imLinea3 = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.047157, imParaProcesar.rows * 0.901330 ),
                              cv::Point( imParaProcesar.cols * 0.953537, imParaProcesar.rows * 0.961197 ) ) );

        ocr->SetImage( imLinea3.data, imLinea3.cols, imLinea3.rows, 3, imLinea3.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.047157, imParaProcesar.rows * 0.901330 ),
                       cv::Point( imParaProcesar.cols * 0.953537, imParaProcesar.rows * 0.961197 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );
    }
    else if ( tipoCarnet == LICENCIA_DORSO )  {

        // Extrae:   DescripcionLinea1 / DescripcionLinea2 / Donante / Sangre / Cuil / Observaciones / Restriccion

        imConRectangulos = imParaProcesar.clone();

        cv::Mat imDescrLinea1 = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.226389, imParaProcesar.rows * 0.256891 ),
                              cv::Point( imParaProcesar.cols * 0.956944, imParaProcesar.rows * 0.293275 ) ) );

        // Le damos la imagen para identificar texto y ejecuta el OCR con GetUTF8Text
        ocr->SetImage( imDescrLinea1.data, imDescrLinea1.cols, imDescrLinea1.rows, 3, imDescrLinea1.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        // Dibuja el rectangulo para registrar la region de la cual fue extraida la informacion
        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.226389, imParaProcesar.rows * 0.256891 ),
                       cv::Point( imParaProcesar.cols * 0.956944, imParaProcesar.rows * 0.293275 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imDescrLinea2 = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.226389, imParaProcesar.rows * 0.293275 ),
                              cv::Point( imParaProcesar.cols * 0.956944, imParaProcesar.rows * 0.334068 ) ) );

        ocr->SetImage( imDescrLinea2.data, imDescrLinea2.cols, imDescrLinea2.rows, 3, imDescrLinea2.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.226389, imParaProcesar.rows * 0.293275 ),
                       cv::Point( imParaProcesar.cols * 0.956944, imParaProcesar.rows * 0.334068 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imDonante = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.305556, imParaProcesar.rows * 0.556781 ),
                              cv::Point( imParaProcesar.cols * 0.340278, imParaProcesar.rows * 0.593164 ) ) );

        ocr->SetImage( imDonante.data, imDonante.cols, imDonante.rows, 3, imDonante.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.305556, imParaProcesar.rows * 0.556781 ),
                       cv::Point( imParaProcesar.cols * 0.340278, imParaProcesar.rows * 0.593164 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imSangre = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.641667, imParaProcesar.rows * 0.555678 ),
                              cv::Point( imParaProcesar.cols * 0.686806, imParaProcesar.rows * 0.590959 ) ) );

        ocr->SetImage( imSangre.data, imSangre.cols, imSangre.rows, 3, imSangre.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.641667, imParaProcesar.rows * 0.555678 ),
                       cv::Point( imParaProcesar.cols * 0.686806, imParaProcesar.rows * 0.590959 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );


        cv::Mat imCuil = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.761806, imParaProcesar.rows * 0.556781 ),
                              cv::Point( imParaProcesar.cols * 0.967361, imParaProcesar.rows * 0.592062 ) ) );

        ocr->SetImage( imCuil.data, imCuil.cols, imCuil.rows, 3, imCuil.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.761806, imParaProcesar.rows * 0.556781 ),
                       cv::Point( imParaProcesar.cols * 0.967361, imParaProcesar.rows * 0.592062 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imObservaciones = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.460417, imParaProcesar.rows * 0.595369 ),
                              cv::Point( imParaProcesar.cols * 0.809722, imParaProcesar.rows * 0.638368 ) ) );

        ocr->SetImage( imObservaciones.data, imObservaciones.cols, imObservaciones.rows, 3, imObservaciones.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.460417, imParaProcesar.rows * 0.595369 ),
                       cv::Point( imParaProcesar.cols * 0.809722, imParaProcesar.rows * 0.638368 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imRestriccion = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.256250, imParaProcesar.rows * 0.639471 ),
                              cv::Point( imParaProcesar.cols * 0.442361, imParaProcesar.rows * 0.670342 ) ) );

        ocr->SetImage( imRestriccion.data, imRestriccion.cols, imRestriccion.rows, 3, imRestriccion.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.256250, imParaProcesar.rows * 0.639471 ),
                       cv::Point( imParaProcesar.cols * 0.442361, imParaProcesar.rows * 0.670342 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );
    }
    else if ( tipoCarnet == VERDE_DORSO )  {
        // Extrae:   Titular / Dni / Domicilio / Localidad

        imConRectangulos = imParaProcesar.clone();

        cv::Mat imTitular = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.250420, imParaProcesar.rows * 0.196335 ),
                              cv::Point( imParaProcesar.cols * 0.672269, imParaProcesar.rows * 0.226003 ) ) );

        // Le damos la imagen para identificar texto y ejecuta el OCR con GetUTF8Text
        ocr->SetImage( imTitular.data, imTitular.cols, imTitular.rows, 3, imTitular.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        // Dibuja el rectangulo para registrar la region de la cual fue extraida la informacion
        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.250420, imParaProcesar.rows * 0.196335 ),
                       cv::Point( imParaProcesar.cols * 0.672269, imParaProcesar.rows * 0.226003 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imDni = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.207283, imParaProcesar.rows * 0.233857 ),
                              cv::Point( imParaProcesar.cols * 0.393277, imParaProcesar.rows * 0.266143 ) ) );

        ocr->SetImage( imDni.data, imDni.cols, imDni.rows, 3, imDni.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.207283, imParaProcesar.rows * 0.233857 ),
                       cv::Point( imParaProcesar.cols * 0.393277, imParaProcesar.rows * 0.266143 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imDomicilio = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.273389, imParaProcesar.rows * 0.372600 ),
                              cv::Point( imParaProcesar.cols * 0.755182, imParaProcesar.rows * 0.405759 ) ) );

        ocr->SetImage( imDomicilio.data, imDomicilio.cols, imDomicilio.rows, 3, imDomicilio.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.273389, imParaProcesar.rows * 0.372600 ),
                       cv::Point( imParaProcesar.cols * 0.755182, imParaProcesar.rows * 0.405759 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imLocalidad = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.140616, imParaProcesar.rows * 0.411867 ),
                              cv::Point( imParaProcesar.cols * 0.371429, imParaProcesar.rows * 0.443281 ) ) );

        ocr->SetImage( imLocalidad.data, imLocalidad.cols, imLocalidad.rows, 3, imLocalidad.step );
        textoExtraido << QString( ocr->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.140616, imParaProcesar.rows * 0.411867 ),
                       cv::Point( imParaProcesar.cols * 0.371429, imParaProcesar.rows * 0.443281 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );
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

        cv::Mat imFoto = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.013483, imParaProcesar.rows * 0.184685 ),
                              cv::Point( imParaProcesar.cols * 0.325843, imParaProcesar.rows * 0.806306 ) ) );

        imFotoPerfil = imFoto.clone();

    }
    else if ( tipoCarnet == LICENCIA )  {

        cv::Mat imFoto = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.013483, imParaProcesar.rows * 0.184685 ),
                              cv::Point( imParaProcesar.cols * 0.318843, imParaProcesar.rows * 0.806306 ) ) );

        imFotoPerfil = imFoto.clone();

    }
    else if ( tipoCarnet == VERDE )  {


    }
    else if ( tipoCarnet == DNI_DORSO )  {


    }
    else if ( tipoCarnet == LICENCIA_DORSO )  {


    }
    else if ( tipoCarnet == VERDE_DORSO )  {


    }
}

//QImage Procesador::extraerFirma(cv::Mat &imParaProcesar, Procesador::TipoCarnet tipoCarnet)
//{
//    return QImage();
//}

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
