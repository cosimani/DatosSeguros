#include "procesador.h"
#include "logger.h"

Procesador * Procesador::instancia = NULL;

Procesador::Procesador( QObject * parent ) : QObject( parent ),
                                             max_features( 500 ),
                                             goodMatchPercent( 0.15f ),
                                             tipoCarnet( NONE ),
                                             ocrLinea( new tesseract::TessBaseAPI ),
                                             ocrParrafo( new tesseract::TessBaseAPI )
{


//    ocr->SetPageSegMode( tesseract::PSM_SINGLE_BLOCK );

    ocrLinea->SetPageSegMode( tesseract::PSM_SINGLE_LINE );
    ocrLinea->SetVariable( "tessedit_char_whitelist", "ABCDEFHIJKLMNOPQRSTUVWXYZ0123456789-<." );

    ocrParrafo->SetPageSegMode( tesseract::PSM_SINGLE_BLOCK );
    ocrParrafo->SetVariable( "tessedit_char_whitelist", "ABCDEFHIJKLMNOPQRSTUVWXYZ0123456789-<." );

//    PSM_AUTO_OSD,       ///< Automatic page segmentation with orientation and
//                        ///< script detection. (OSD)
//    PSM_AUTO_ONLY,      ///< Automatic page segmentation, but no OSD, or OCR.
//    PSM_AUTO,           ///< Fully automatic page segmentation, but no OSD.
//    PSM_SINGLE_COLUMN,  ///< Assume a single column of text of variable sizes.
//    PSM_SINGLE_BLOCK_VERT_TEXT,  ///< Assume a single uniform block of vertically
//                                 ///< aligned text.
//    PSM_SINGLE_BLOCK,   ///< Assume a single uniform block of text. (Default.)
//    PSM_SINGLE_LINE,    ///< Treat the image as a single text line.
//    PSM_SINGLE_WORD,    ///< Treat the image as a single word.
//    PSM_CIRCLE_WORD,    ///< Treat the image as a single word in a circle.
//    PSM_SINGLE_CHAR,    ///< Treat the image as a single character.
//    PSM_SPARSE_TEXT,    ///< Find as much text as possible in no particular order.
//    PSM_SPARSE_TEXT_OSD,  ///< Sparse text with orientation and script det.


    // Initialize tesseract to use Spanish (spa) and the LSTM OCR engine.
    ocrLinea->Init( NULL, "spa", tesseract::OEM_LSTM_ONLY );
    ocrParrafo->Init( NULL, "spa", tesseract::OEM_LSTM_ONLY );



    // Set Page segmentation mode to PSM_AUTO (3)
//    ocr->SetPageSegMode( tesseract::PSM_AUTO );

//    ocr->Init( NULL, "spa", tesseract::OEM_DEFAULT );




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

void Procesador::alinearOtraOpcion( const cv::Mat & imParaAlinear, cv::Mat & imAlineada, TipoCarnet tipoCarnet )  {
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

}


bool Procesador::alinearSurfFlann( const cv::Mat & imParaAlinear, cv::Mat & imAlineada, TipoCarnet tipoCarnet )  {

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
        return false;
    }

    // img_object es la referencia im2Gray
    // img_scene es la imagen recibida / o sea, es imParaAlinear = im1Gray

    //-- Step 1: Detect the keypoints using SURF Detector, compute the descriptors
    int minHessian = 400;
    cv::Ptr< cv::xfeatures2d::SURF > detector = cv::xfeatures2d::SURF::create( minHessian );
    std::vector< cv::KeyPoint > keypoints_object, keypoints_scene;
    cv::Mat descriptors_object, descriptors_scene;
    detector->detectAndCompute( im2Gray, cv::noArray(), keypoints_object, descriptors_object );
    detector->detectAndCompute( im1Gray, cv::noArray(), keypoints_scene, descriptors_scene );

    if ( keypoints_scene.size() < 4 || keypoints_object.size() < 4 )  {
        QString str = "No hay suficientes keypoints " + QString::number( keypoints_scene.size() )
                + " y " + QString::number( keypoints_object.size() );
        qDebug() << str;
        LOG_WAR( str );
        return false;
    }

    //-- Step 2: Matching descriptor vectors with a FLANN based matcher
    // Since SURF is a floating-point descriptor NORM_L2 is used
    cv::Ptr< cv::DescriptorMatcher > matcher = cv::DescriptorMatcher::create( cv::DescriptorMatcher::FLANNBASED );
    std::vector< std::vector< cv::DMatch > > knn_matches;
    matcher->knnMatch( descriptors_object, descriptors_scene, knn_matches, 2 );

    //-- Filter matches using the Lowe's ratio test
    const float ratio_thresh = 0.75f;

    std::vector< cv::DMatch > good_matches;

    for (size_t i = 0; i < knn_matches.size(); i++)
    {
        if ( knn_matches[ i ][ 0 ].distance < ratio_thresh * knn_matches[ i ][ 1 ].distance )
        {
            good_matches.push_back( knn_matches[ i ][ 0 ] );
        }
    }

    unsigned int cantidadMinimaMatchesRequerida = 0;

    if ( tipoCarnet == DNI )  {
        cantidadMinimaMatchesRequerida = 500;
    }
    else if ( tipoCarnet == DNI_DORSO )  {
        cantidadMinimaMatchesRequerida = 500;
    }
    else if ( tipoCarnet == LICENCIA )  {
        cantidadMinimaMatchesRequerida = 500;
    }
    else if ( tipoCarnet == LICENCIA_DORSO )  {
        cantidadMinimaMatchesRequerida = 500;
    }
    else  {
        cantidadMinimaMatchesRequerida = 500;
    }


    if ( good_matches.size() < cantidadMinimaMatchesRequerida )  {
        QString str = "Good matches:" + QString::number( good_matches.size() ) + ". No suficientes para alinear la imagen";
        qDebug() << str;
        LOG_WAR( str );
        return false;
    }
    else  {
        QString str = "Good matches:" + QString::number( good_matches.size() ) + ". Son suficientes para alinear la imagen";
        qDebug() << str;
        LOG_INF( str );
    }

    //-- Draw matches
    cv::Mat img_matches;
    cv::drawMatches( im2Gray, keypoints_object, im1Gray, keypoints_scene, good_matches, img_matches, cv::Scalar::all(-1),
                     cv::Scalar::all(-1), std::vector<char>(), cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS );

    //-- Localize the object
    std::vector< cv::Point2f > obj;
    std::vector< cv::Point2f > scene;
    for( size_t i = 0; i < good_matches.size(); i++ )
    {
        //-- Get the keypoints from the good matches
        obj.push_back( keypoints_object[ good_matches[i].queryIdx ].pt );
        scene.push_back( keypoints_scene[ good_matches[i].trainIdx ].pt );
    }
//    cv::Mat H = cv::findHomography( obj, scene, cv::RANSAC );
    cv::Mat H = cv::findHomography( scene, obj, cv::RANSAC );

    //-- Get the corners from the image_1 ( the object to be "detected" )
    std::vector< cv::Point2f > obj_corners( 4 );
    obj_corners[ 0 ] = cv::Point2f( 0, 0 );
    obj_corners[ 1 ] = cv::Point2f( ( float )im2Gray.cols, 0 );
    obj_corners[ 2 ] = cv::Point2f( ( float )im2Gray.cols, ( float )im2Gray.rows );
    obj_corners[ 3 ] = cv::Point2f( 0, ( float )im2Gray.rows );
    std::vector< cv::Point2f > scene_corners( 4 );
//    cv::perspectiveTransform( obj_corners, scene_corners, H );
    cv::perspectiveTransform( scene_corners, obj_corners, H );

    //-- Draw lines between the corners (the mapped object in the scene - image_2 )
    cv::line( img_matches, scene_corners[ 0 ] + cv::Point2f( ( float )im2Gray.cols, 0 ),
              scene_corners[ 1 ] + cv::Point2f( ( float )im2Gray.cols, 0 ), cv::Scalar( 0, 255, 0 ), 4 );
    cv::line( img_matches, scene_corners[ 1 ] + cv::Point2f( ( float )im2Gray.cols, 0 ),
              scene_corners[ 2 ] + cv::Point2f( ( float )im2Gray.cols, 0 ), cv::Scalar( 0, 255, 0 ), 4 );
    cv::line( img_matches, scene_corners[ 2 ] + cv::Point2f( ( float )im2Gray.cols, 0 ),
              scene_corners[ 3 ] + cv::Point2f( ( float )im2Gray.cols, 0 ), cv::Scalar( 0, 255, 0 ), 4 );
    cv::line( img_matches, scene_corners[ 3 ] + cv::Point2f( ( float )im2Gray.cols, 0 ),
              scene_corners[ 0 ] + cv::Point2f( ( float )im2Gray.cols, 0 ), cv::Scalar( 0, 255, 0 ), 4 );

    //-- Write image with detected matches
    cv::imwrite( "imagenes/registros/matches.jpg", img_matches );

    if ( tipoCarnet == DNI )  {
        cv::warpPerspective( imParaAlinear, imAlineada, H, this->imReferenciaDNI.size() );
    }
    else if ( tipoCarnet == LICENCIA )  {
        cv::warpPerspective( imParaAlinear, imAlineada, H, this->imReferenciaLicencia.size() );
    }
    else if ( tipoCarnet == VERDE )  {
        cv::warpPerspective( imParaAlinear, imAlineada, H, this->imReferenciaVerde.size() );
    }
    else if ( tipoCarnet == DNI_DORSO )  {
        cv::warpPerspective( imParaAlinear, imAlineada, H, this->imReferenciaDNI_dorso.size() );
    }
    else if ( tipoCarnet == LICENCIA_DORSO )  {
        cv::warpPerspective( imParaAlinear, imAlineada, H, this->imReferenciaLicencia_dorso.size() );
    }
    else if ( tipoCarnet == VERDE_DORSO )  {
        cv::warpPerspective( imParaAlinear, imAlineada, H, this->imReferenciaVerde_dorso.size() );
    }
    else  {
        return false;
    }

    return true;

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


    /// Opcion 2
//    std::vector< std::vector< cv::DMatch > > matches;
//    //using either FLANN or BruteForce
//    cv::Ptr< cv::DescriptorMatcher > matcher = cv::DescriptorMatcher::create( "BruteForceMatcher" );
//    matcher->knnMatch( descriptors1, descriptors2, matches, 1 );

//    //just some temporarily code to have the right data structure
//    std::vector< cv::DMatch > good_matches2;
//    good_matches2.reserve( matches.size() );
//    for ( size_t i = 0; i < matches.size(); ++i )
//    {
//        good_matches2.push_back( matches[ i ][ 0 ] );
//    }





    /// Opcion 1
//    cv::BFMatcher matcher;
    cv::BFMatcher matcher( cv::NORM_L2 );
    std::vector< cv::DMatch > matches;
    matcher.match( descriptors1, descriptors2, matches );

    // Sort matches by score
    std::sort( matches.begin(), matches.end() );

    // Remove not so good matches
    const int numGoodMatches = matches.size() * this->goodMatchPercent;  // 0.15f
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

    cv::imwrite( "imagenes/registros/matches.jpg", imMatches );

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


QString Procesador::extraerTexto( const cv::Mat & imParaProcesar, Procesador::TipoCarnet tipoCarnet )
{
    QString textoExtraido;

    if ( tipoCarnet == DNI_DORSO )  {

        // Le damos la imagen para identificar texto y ejecuta el OCR con GetUTF8Text
        ocrLinea->SetImage( imParaProcesar.data, imParaProcesar.cols, imParaProcesar.rows, 3, imParaProcesar.step );
        textoExtraido.append( ocrLinea->GetUTF8Text() );

    }

    return textoExtraido;
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
        // Extrae:   dni / apellido / nombre / sexo / fecha_nac / fecha_otorg / fecha_venc / ejemplar

//        int wDni = imReferenciaDNI.cols;
//        int hDni = imReferenciaDNI.rows;
//        cv::Point pDni_up( 86 * 100 / wDni, 1319 * 100 / hDni );  // Es el pixel de arriba izquierda
//        cv::Point pDni_down( 697 * 100 / wDni, 1465 * 100 / hDni );  // Es el pixel de abajo derecha
//        cv::Point pApellido_up( 849 * 100 / wDni, 336 * 100 / hDni );
//        cv::Point pApellido_down( 1341 * 100 / wDni, 413 * 100 / hDni );
//        cv::Point pNombre_up( 840 * 100 / wDni, 535 * 100 / hDni );
//        cv::Point pNombre_down( 1538 * 100 / wDni, 624 * 100 / hDni );
//        cv::Point pDomicilio_up( 848 * 100 / wDni, 743 * 100 / hDni );
//        cv::Point pDomicilio_down( 961 * 100 / wDni, 814 * 100 / hDni );
//        cv::Point pFechaNac_up( 829 * 100 / wDni, 880 * 100 / hDni );
//        cv::Point pFechaNac_down( 1462 * 100 / wDni, 951 * 100 / hDni );
//        cv::Point pFechaOtorg_up( 849 * 100 / wDni, 1021 * 100 / hDni );
//        cv::Point pFechaOtorg_down( 1468 * 100 / wDni, 1090 * 100 / hDni );
//        cv::Point pFechaVenc_up( 844 * 100 / wDni, 1160 * 100 / hDni );
//        cv::Point pFechaVenc_down( 1469 * 100 / wDni, 1223 * 100 / hDni );
//        cv::Point pClase_up( 1730 * 100 / wDni, 752 * 100 / hDni );
//        cv::Point pClase_down( 1814 * 100 / wDni, 813 * 100 / hDni );
//        cv::Point pFoto_up( 142 * 100 / wDni, 378 * 100 / hDni );
//        cv::Point pFoto_down( 818 * 100 / wDni, 1214 * 100 / hDni );

        cv::Point pDni_up( 86, 1319 );  // Es el pixel de arriba izquierda
        cv::Point pDni_down( 697, 1465 );  // Es el pixel de abajo derecha
        cv::Point pApellido_up( 849, 336 );
        cv::Point pApellido_down( 1341, 413 );
        cv::Point pNombre_up( 840, 535 );
        cv::Point pNombre_down( 1538, 624 );
        cv::Point pDomicilio_up( 848, 743 );
        cv::Point pDomicilio_down( 961, 814 );
        cv::Point pFechaNac_up( 829, 880 );
        cv::Point pFechaNac_down( 1462, 951 );
        cv::Point pFechaOtorg_up( 849, 1021 );
        cv::Point pFechaOtorg_down( 1468, 1090 );
        cv::Point pFechaVenc_up( 844, 1160 );
        cv::Point pFechaVenc_down( 1469, 1223 );
        cv::Point pClase_up( 1730, 752 );
        cv::Point pClase_down( 1814, 813 );
        cv::Point pFoto_up( 142, 378 );
        cv::Point pFoto_down( 818, 1214 );

        imConRectangulos = imParaProcesar.clone();

        cv::Mat imDni = imParaProcesar( cv::Rect( pDni_up, pDni_down ) );

        // Le damos la imagen para identificar texto y ejecuta el OCR con GetUTF8Text
        ocrLinea->SetImage( imDni.data, imDni.cols, imDni.rows, 3, imDni.step );
        textoExtraido << QString( ocrLinea->GetUTF8Text() );

        // Dibuja el rectangulo para registrar la region de la cual fue extraida la informacion
        cv::rectangle( imConRectangulos, pDni_up, pDni_down, cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imApellido = imParaProcesar( cv::Rect( pApellido_up, pApellido_down ) );

        ocrLinea->SetImage( imApellido.data, imApellido.cols, imApellido.rows, 3, imApellido.step );
        textoExtraido << QString( ocrLinea->GetUTF8Text() );

        cv::rectangle( imConRectangulos, pApellido_up, pApellido_down, cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imNombre = imParaProcesar( cv::Rect( pNombre_up, pNombre_down ) );

        ocrLinea->SetImage( imNombre.data, imNombre.cols, imNombre.rows, 3, imNombre.step );
        textoExtraido << QString( ocrLinea->GetUTF8Text() );

        cv::rectangle( imConRectangulos, pNombre_up, pNombre_down, cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        // Este es el dato "Sexo"
        cv::Mat imDomicilio = imParaProcesar( cv::Rect( pDomicilio_up, pDomicilio_down ) );

        ocrLinea->SetImage( imDomicilio.data, imDomicilio.cols, imDomicilio.rows, 3, imDomicilio.step );
        textoExtraido << QString( ocrLinea->GetUTF8Text() );

        cv::rectangle( imConRectangulos, pDomicilio_up, pDomicilio_down, cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );


        cv::Mat imFechaNac = imParaProcesar( cv::Rect( pFechaNac_up, pFechaNac_down ) );

        ocrLinea->SetImage( imFechaNac.data, imFechaNac.cols, imFechaNac.rows, 3, imFechaNac.step );
        textoExtraido << QString( ocrLinea->GetUTF8Text() );

        cv::rectangle( imConRectangulos, pFechaNac_up, pFechaNac_down, cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imFechaOtorg = imParaProcesar( cv::Rect( pFechaOtorg_up, pFechaOtorg_down ) );

        ocrLinea->SetImage( imFechaOtorg.data, imFechaOtorg.cols, imFechaOtorg.rows, 3, imFechaOtorg.step );
        textoExtraido << QString( ocrLinea->GetUTF8Text() );

        cv::rectangle( imConRectangulos, pFechaOtorg_up, pFechaOtorg_down, cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imFechaVenc = imParaProcesar( cv::Rect( pFechaVenc_up, pFechaVenc_down ) );

        ocrLinea->SetImage( imFechaVenc.data, imFechaVenc.cols, imFechaVenc.rows, 3, imFechaVenc.step );
        textoExtraido << QString( ocrLinea->GetUTF8Text() );

        cv::rectangle( imConRectangulos, pFechaVenc_up, pFechaVenc_down, cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );


//        cv::Mat imFoto = imParaProcesar(
//                    cv::Rect( cv::Point( imParaProcesar.cols * 0.054601, imParaProcesar.rows * 0.223529 ),
//                              cv::Point( imParaProcesar.cols * 0.352761, imParaProcesar.rows * 0.827451 ) ) );

        cv::rectangle( imConRectangulos, pFoto_up, pFoto_down, cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        // Este es el dato "Ejemplar"
        cv::Mat imClase = imParaProcesar( cv::Rect( pClase_up, pClase_down ) );

        ocrLinea->SetImage( imClase.data, imClase.cols, imClase.rows, 3, imClase.step );
        textoExtraido << QString( ocrLinea->GetUTF8Text() );

        cv::rectangle( imConRectangulos, pClase_up, pClase_down, cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );


    }
    else if ( tipoCarnet == DNI_DORSO )  {
        // Extrae:   domicilio / lugar_nac / cuil / linea1 / linea2 / linea3
        // Extrae:   domicilioLinea1 / lineas / domicilioLinea2 / domicilioLinea3 (lugar de nacimiento)

        cv::Point pDomicilio_up( 392, 166 );  // Es el pixel de arriba izquierda
        cv::Point pDomicilio_down( 1800, 236 );  // Es el pixel de abajo derecha
        cv::Point pLugarNac_up( 110, 234 );
        cv::Point pLugarNac_down( 1520, 300 );
        cv::Point pCuil_up( 262, 962 );
        cv::Point pCuil_down( 792, 1032 );
        cv::Point pLinea1_up( 92, 1220 );
        cv::Point pLinea1_down( 2466, 1334 );
        cv::Point pLinea2_up( 92, 1334 );
        cv::Point pLinea2_down( 2466, 1462 );
        cv::Point pLinea3_up( 92, 1462 );
        cv::Point pLinea3_down( 2466, 1586 );

        cv::Point parrafoDomicilioLinea1_up( 380, 132 );  // Es el pixel de arriba izquierda
        cv::Point parrafoDomicilioLinea1_down( 1959, 405 );  // Es el pixel de abajo derecha

        cv::Point parrafoDomicilioLinea2_up( 80, 132 );
        cv::Point parrafoDomicilioLinea2_down( 1959, 405 );

        cv::Point parrafoDomicilioLinea3_up( 684, 132 );
        cv::Point parrafoDomicilioLinea3_down( 1959, 405 );


        cv::Point parrafoLineas_up( 36, 1148 );
        cv::Point parrafoLineas_down( 2500, 1612 );


        imConRectangulos = imParaProcesar.clone();

//        cv::Mat imDomicilio = imParaProcesar( cv::Rect( pDomicilio_up, pDomicilio_down ) );

//        // Le damos la imagen para identificar texto y ejecuta el OCR con GetUTF8Text
//        ocrLinea->SetImage( imDomicilio.data, imDomicilio.cols, imDomicilio.rows, 3, imDomicilio.step );
//        textoExtraido << QString( ocrLinea->GetUTF8Text() );

//        // Dibuja el rectangulo para registrar la region de la cual fue extraida la informacion
//        cv::rectangle( imConRectangulos, pDomicilio_up, pDomicilio_down, cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );


        cv::Mat imParrafoDomicilio1 = imParaProcesar( cv::Rect( parrafoDomicilioLinea1_up, parrafoDomicilioLinea1_down ) );

        // Le damos la imagen para identificar texto y ejecuta el OCR con GetUTF8Text
        ocrParrafo->SetImage( imParrafoDomicilio1.data, imParrafoDomicilio1.cols,
                              imParrafoDomicilio1.rows, 3, imParrafoDomicilio1.step );
        textoExtraido << QString( ocrParrafo->GetUTF8Text() );

        // Dibuja el rectangulo para registrar la region de la cual fue extraida la informacion
        cv::rectangle( imConRectangulos, parrafoDomicilioLinea1_up, parrafoDomicilioLinea1_down,
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

//        cv::Mat imLugarNac = imParaProcesar( cv::Rect( pLugarNac_up, pLugarNac_down ) );

//        // Le damos la imagen para identificar texto y ejecuta el OCR con GetUTF8Text
//        ocrLinea->SetImage( imLugarNac.data, imLugarNac.cols, imLugarNac.rows, 3, imLugarNac.step );
//        textoExtraido << QString( ocrLinea->GetUTF8Text() );

//        // Dibuja el rectangulo para registrar la region de la cual fue extraida la informacion
//        cv::rectangle( imConRectangulos, pLugarNac_up, pLugarNac_down, cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );


//        cv::Mat imCuil = imParaProcesar( cv::Rect( pCuil_up, pCuil_down ) );

//        // Le damos la imagen para identificar texto y ejecuta el OCR con GetUTF8Text
//        ocrLinea->SetImage( imCuil.data, imCuil.cols, imCuil.rows, 3, imCuil.step );
//        textoExtraido << QString( ocrLinea->GetUTF8Text() );

//        // Dibuja el rectangulo para registrar la region de la cual fue extraida la informacion
//        cv::rectangle( imConRectangulos, pCuil_up, pCuil_down, cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );



//        cv::Mat imLinea1 = imParaProcesar( cv::Rect( pLinea1_up, pLinea1_down ) );

//        // Le damos la imagen para identificar texto y ejecuta el OCR con GetUTF8Text
//        ocrLinea->SetImage( imLinea1.data, imLinea1.cols, imLinea1.rows, 3, imLinea1.step );
//        textoExtraido << QString( ocrLinea->GetUTF8Text() );

//        // Dibuja el rectangulo para registrar la region de la cual fue extraida la informacion
//        cv::rectangle( imConRectangulos, pLinea1_up, pLinea1_down, cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );


//        cv::Mat imLinea2 = imParaProcesar( cv::Rect( pLinea2_up, pLinea2_down ) );

//        // Le damos la imagen para identificar texto y ejecuta el OCR con GetUTF8Text
//        ocrLinea->SetImage( imLinea2.data, imLinea2.cols, imLinea2.rows, 3, imLinea2.step );
//        textoExtraido << QString( ocrLinea->GetUTF8Text() );

//        // Dibuja el rectangulo para registrar la region de la cual fue extraida la informacion
//        cv::rectangle( imConRectangulos, pLinea2_up, pLinea2_down, cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );


//        cv::Mat imLinea3 = imParaProcesar( cv::Rect( pLinea3_up, pLinea3_down ) );

//        // Le damos la imagen para identificar texto y ejecuta el OCR con GetUTF8Text
//        ocrLinea->SetImage( imLinea3.data, imLinea3.cols, imLinea3.rows, 3, imLinea3.step );
//        textoExtraido << QString( ocrLinea->GetUTF8Text() );

//        // Dibuja el rectangulo para registrar la region de la cual fue extraida la informacion
//        cv::rectangle( imConRectangulos, pLinea3_up, pLinea3_down, cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );


        cv::Mat imLineas = imParaProcesar( cv::Rect( parrafoLineas_up, parrafoLineas_down ) );

        // Le damos la imagen para identificar texto y ejecuta el OCR con GetUTF8Text
        ocrParrafo->SetImage( imLineas.data, imLineas.cols, imLineas.rows, 3, imLineas.step );
        textoExtraido << QString( ocrParrafo->GetUTF8Text() );

        // Dibuja el rectangulo para registrar la region de la cual fue extraida la informacion
        cv::rectangle( imConRectangulos, parrafoLineas_up, parrafoLineas_down, cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );


        cv::Mat imParrafoDomicilio2 = imParaProcesar( cv::Rect( parrafoDomicilioLinea2_up, parrafoDomicilioLinea2_down ) );
        ocrParrafo->SetImage( imParrafoDomicilio2.data, imParrafoDomicilio2.cols,
                              imParrafoDomicilio2.rows, 3, imParrafoDomicilio2.step );
        textoExtraido << QString( ocrParrafo->GetUTF8Text() );
        cv::rectangle( imConRectangulos, parrafoDomicilioLinea2_up, parrafoDomicilioLinea2_down,
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imParrafoDomicilio3 = imParaProcesar( cv::Rect( parrafoDomicilioLinea3_up, parrafoDomicilioLinea3_down ) );
        ocrParrafo->SetImage( imParrafoDomicilio3.data, imParrafoDomicilio3.cols,
                              imParrafoDomicilio3.rows, 3, imParrafoDomicilio3.step );
        textoExtraido << QString( ocrParrafo->GetUTF8Text() );
        cv::rectangle( imConRectangulos, parrafoDomicilioLinea3_up, parrafoDomicilioLinea3_down,
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );


    }
    else if ( tipoCarnet == LICENCIA )  {

        // Extrae:   dni / apellido / nombre / domicilio / fecha_nac / fecha_otorg / fecha_venc / clase

        cv::Point pDni_up( 753, 327 );  // Es el pixel de arriba izquierda
        cv::Point pDni_down( 1365, 447 );  // Es el pixel de abajo derecha
        cv::Point pApellido_up( 771, 480 );
        cv::Point pApellido_down( 1659, 585 );
        cv::Point pNombre_up( 780, 630 );
        cv::Point pNombre_down( 2058, 735 );
        cv::Point pDomicilio_up( 777, 771 );
        cv::Point pDomicilio_down( 2259, 858 );
        cv::Point pFecha_nac_up( 777, 984 );
        cv::Point pFecha_nac_down( 1434, 1080 );
        cv::Point pFecha_otorg_up( 777, 1134 );
        cv::Point pFecha_otorg_down( 1470, 1230 );
        cv::Point pFecha_venc_up( 1536, 1128 );
        cv::Point pFecha_venc_down( 2337, 1242 );
        cv::Point pClase_up( 2118, 333 );
        cv::Point pClase_down( 2340, 468 );

        imConRectangulos = imParaProcesar.clone();

        cv::Mat imDni = imParaProcesar( cv::Rect( pDni_up, pDni_down ) );
        ocrLinea->SetImage( imDni.data, imDni.cols, imDni.rows, 3, imDni.step );
        textoExtraido << QString( ocrLinea->GetUTF8Text() );
        cv::rectangle( imConRectangulos, pDni_up, pDni_down, cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imApellido = imParaProcesar( cv::Rect( pApellido_up, pApellido_down ) );
        ocrLinea->SetImage( imApellido.data, imApellido.cols, imApellido.rows, 3, imApellido.step );
        textoExtraido << QString( ocrLinea->GetUTF8Text() );
        cv::rectangle( imConRectangulos, pApellido_up, pApellido_down, cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imNombre = imParaProcesar( cv::Rect( pNombre_up, pNombre_down ) );
        ocrLinea->SetImage( imNombre.data, imNombre.cols, imNombre.rows, 3, imNombre.step );
        textoExtraido << QString( ocrLinea->GetUTF8Text() );
        cv::rectangle( imConRectangulos, pNombre_up, pNombre_down, cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imDomicilio = imParaProcesar( cv::Rect( pDomicilio_up, pDomicilio_down ) );
        ocrLinea->SetImage( imDomicilio.data, imDomicilio.cols, imDomicilio.rows, 3, imDomicilio.step );
        textoExtraido << QString( ocrLinea->GetUTF8Text() );
        cv::rectangle( imConRectangulos, pDomicilio_up, pDomicilio_down, cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imFecha_nac = imParaProcesar( cv::Rect( pFecha_nac_up, pFecha_nac_down ) );
        ocrLinea->SetImage( imFecha_nac.data, imFecha_nac.cols, imFecha_nac.rows, 3, imFecha_nac.step );
        textoExtraido << QString( ocrLinea->GetUTF8Text() );
        cv::rectangle( imConRectangulos, pFecha_nac_up, pFecha_nac_down, cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imFecha_otorg = imParaProcesar( cv::Rect( pFecha_otorg_up, pFecha_otorg_down ) );
        ocrLinea->SetImage( imFecha_otorg.data, imFecha_otorg.cols, imFecha_otorg.rows, 3, imFecha_otorg.step );
        textoExtraido << QString( ocrLinea->GetUTF8Text() );
        cv::rectangle( imConRectangulos, pFecha_otorg_up, pFecha_otorg_down, cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imFecha_venc = imParaProcesar( cv::Rect( pFecha_venc_up, pFecha_venc_down ) );
        ocrLinea->SetImage( imFecha_venc.data, imFecha_venc.cols, imFecha_venc.rows, 3, imFecha_venc.step );
        textoExtraido << QString( ocrLinea->GetUTF8Text() );
        cv::rectangle( imConRectangulos, pFecha_venc_up, pFecha_venc_down, cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imClase = imParaProcesar( cv::Rect( pClase_up, pClase_down ) );
        ocrLinea->SetImage( imClase.data, imClase.cols, imClase.rows, 3, imClase.step );
        textoExtraido << QString( ocrLinea->GetUTF8Text() );
        cv::rectangle( imConRectangulos, pClase_up, pClase_down, cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );
    }
    else if ( tipoCarnet == LICENCIA_DORSO )  {

        // Extrae:   clase / descripcionClase / descr_observ

        cv::Point pClase_up( 345, 348 );
        cv::Point pClase_down( 459, 447 );
        cv::Point pdescrClase_up( 453, 20 );
        cv::Point pdescrClase_down( 2070, 534 );
        cv::Point pDescr_observ_up( 225, 693 );
        cv::Point pDescr_observ_down( 2097, 939 );

        imConRectangulos = imParaProcesar.clone();

        cv::Mat imClase = imParaProcesar( cv::Rect( pClase_up, pClase_down ) );
        ocrLinea->SetImage( imClase.data, imClase.cols, imClase.rows, 3, imClase.step );
        textoExtraido << QString( ocrLinea->GetUTF8Text() );
        cv::rectangle( imConRectangulos, pClase_up, pClase_down, cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imDescrClase = imParaProcesar( cv::Rect( pdescrClase_up, pdescrClase_down ) );
        ocrParrafo->SetImage( imDescrClase.data, imDescrClase.cols, imDescrClase.rows, 3, imDescrClase.step );
        textoExtraido << QString( ocrParrafo->GetUTF8Text() );
        cv::rectangle( imConRectangulos, pdescrClase_up, pdescrClase_down, cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imDescr_observ = imParaProcesar( cv::Rect( pDescr_observ_up, pDescr_observ_down ) );
        ocrParrafo->SetImage( imDescr_observ.data, imDescr_observ.cols, imDescr_observ.rows, 3, imDescr_observ.step );
        textoExtraido << QString( ocrParrafo->GetUTF8Text() );
        cv::rectangle( imConRectangulos, pDescr_observ_up, pDescr_observ_down, cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

    }
    else if ( tipoCarnet == VERDE )  {

        // Extrae:   Dominio / Modelo / Tipo / Uso / Chasis / Motor / Vencimiento / Marca

        imConRectangulos = imParaProcesar.clone();

        cv::Mat imDominio = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.314206, imParaProcesar.rows * 0.300778 ),
                              cv::Point( imParaProcesar.cols * 0.485237, imParaProcesar.rows * 0.336214 ) ) );

        // Le damos la imagen para identificar texto y ejecuta el OCR con GetUTF8Text
        ocrLinea->SetImage( imDominio.data, imDominio.cols, imDominio.rows, 3, imDominio.step );
        textoExtraido << QString( ocrLinea->GetUTF8Text() );

        // Dibuja el rectangulo para registrar la region de la cual fue extraida la informacion
        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.314206, imParaProcesar.rows * 0.300778 ),
                       cv::Point( imParaProcesar.cols * 0.485237, imParaProcesar.rows * 0.336214 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imModelo = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.310306, imParaProcesar.rows * 0.391530 ),
                              cv::Point( imParaProcesar.cols * 0.523120, imParaProcesar.rows * 0.432152 ) ) );

        ocrLinea->SetImage( imModelo.data, imModelo.cols, imModelo.rows, 3, imModelo.step );
        textoExtraido << QString( ocrLinea->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.310306, imParaProcesar.rows * 0.391530 ),
                       cv::Point( imParaProcesar.cols * 0.523120, imParaProcesar.rows * 0.432152 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imTipo = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.261281, imParaProcesar.rows * 0.439931 ),
                              cv::Point( imParaProcesar.cols * 0.509749, imParaProcesar.rows * 0.477960 ) ) );

        ocrLinea->SetImage( imTipo.data, imTipo.cols, imTipo.rows, 3, imTipo.step );
        textoExtraido << QString( ocrLinea->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.261281, imParaProcesar.rows * 0.439931 ),
                       cv::Point( imParaProcesar.cols * 0.509749, imParaProcesar.rows * 0.477960 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imUso = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.257939, imParaProcesar.rows * 0.490061 ),
                              cv::Point( imParaProcesar.cols * 0.389972, imParaProcesar.rows * 0.526361 ) ) );

        ocrLinea->SetImage( imUso.data, imUso.cols, imUso.rows, 3, imUso.step );
        textoExtraido << QString( ocrLinea->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.257939, imParaProcesar.rows * 0.490061 ),
                       cv::Point( imParaProcesar.cols * 0.389972, imParaProcesar.rows * 0.526361 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );


        cv::Mat imChasis = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.304178, imParaProcesar.rows * 0.540190 ),
                              cv::Point( imParaProcesar.cols * 0.585515, imParaProcesar.rows * 0.574762 ) ) );

        ocrLinea->SetImage( imChasis.data, imChasis.cols, imChasis.rows, 3, imChasis.step );
        textoExtraido << QString( ocrLinea->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.304178, imParaProcesar.rows * 0.540190 ),
                       cv::Point( imParaProcesar.cols * 0.585515, imParaProcesar.rows * 0.574762 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imMotor = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.300279, imParaProcesar.rows * 0.588591 ),
                              cv::Point( imParaProcesar.cols * 0.537604, imParaProcesar.rows * 0.625756 ) ) );

        ocrLinea->SetImage( imMotor.data, imMotor.cols, imMotor.rows, 3, imMotor.step );
        textoExtraido << QString( ocrLinea->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.300279, imParaProcesar.rows * 0.588591 ),
                       cv::Point( imParaProcesar.cols * 0.537604, imParaProcesar.rows * 0.625756 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imVencimiento = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.293593, imParaProcesar.rows * 0.634399 ),
                              cv::Point( imParaProcesar.cols * 0.457382, imParaProcesar.rows * 0.670700 ) ) );

        ocrLinea->SetImage( imVencimiento.data, imVencimiento.cols, imVencimiento.rows, 3, imVencimiento.step );
        textoExtraido << QString( ocrLinea->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.293593, imParaProcesar.rows * 0.634399 ),
                       cv::Point( imParaProcesar.cols * 0.457382, imParaProcesar.rows * 0.670700 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imMarca = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.289694, imParaProcesar.rows * 0.344857 ),
                              cv::Point( imParaProcesar.cols * 0.448468, imParaProcesar.rows * 0.382022 ) ) );

        ocrLinea->SetImage( imMarca.data, imMarca.cols, imMarca.rows, 3, imMarca.step );
        textoExtraido << QString( ocrLinea->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.289694, imParaProcesar.rows * 0.344857 ),
                       cv::Point( imParaProcesar.cols * 0.448468, imParaProcesar.rows * 0.382022 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );
    }
    else if ( tipoCarnet == VERDE_DORSO )  {
        // Extrae:   Titular / Dni / Domicilio / Localidad

        imConRectangulos = imParaProcesar.clone();

        cv::Mat imTitular = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.250420, imParaProcesar.rows * 0.196335 ),
                              cv::Point( imParaProcesar.cols * 0.672269, imParaProcesar.rows * 0.226003 ) ) );

        // Le damos la imagen para identificar texto y ejecuta el OCR con GetUTF8Text
        ocrLinea->SetImage( imTitular.data, imTitular.cols, imTitular.rows, 3, imTitular.step );
        textoExtraido << QString( ocrLinea->GetUTF8Text() );

        // Dibuja el rectangulo para registrar la region de la cual fue extraida la informacion
        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.250420, imParaProcesar.rows * 0.196335 ),
                       cv::Point( imParaProcesar.cols * 0.672269, imParaProcesar.rows * 0.226003 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imDni = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.207283, imParaProcesar.rows * 0.233857 ),
                              cv::Point( imParaProcesar.cols * 0.393277, imParaProcesar.rows * 0.266143 ) ) );

        ocrLinea->SetImage( imDni.data, imDni.cols, imDni.rows, 3, imDni.step );
        textoExtraido << QString( ocrLinea->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.207283, imParaProcesar.rows * 0.233857 ),
                       cv::Point( imParaProcesar.cols * 0.393277, imParaProcesar.rows * 0.266143 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imDomicilio = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.273389, imParaProcesar.rows * 0.372600 ),
                              cv::Point( imParaProcesar.cols * 0.755182, imParaProcesar.rows * 0.405759 ) ) );

        ocrLinea->SetImage( imDomicilio.data, imDomicilio.cols, imDomicilio.rows, 3, imDomicilio.step );
        textoExtraido << QString( ocrLinea->GetUTF8Text() );

        cv::rectangle( imConRectangulos,
                       cv::Point( imParaProcesar.cols * 0.273389, imParaProcesar.rows * 0.372600 ),
                       cv::Point( imParaProcesar.cols * 0.755182, imParaProcesar.rows * 0.405759 ),
                       cv::Scalar( 0, 0, 255 ), sizeBordeRectangulo );

        cv::Mat imLocalidad = imParaProcesar(
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.140616, imParaProcesar.rows * 0.411867 ),
                              cv::Point( imParaProcesar.cols * 0.371429, imParaProcesar.rows * 0.443281 ) ) );

        ocrLinea->SetImage( imLocalidad.data, imLocalidad.cols, imLocalidad.rows, 3, imLocalidad.step );
        textoExtraido << QString( ocrLinea->GetUTF8Text() );

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
                    cv::Rect( cv::Point( imParaProcesar.cols * 0.054601, imParaProcesar.rows * 0.223529 ),
                              cv::Point( imParaProcesar.cols * 0.352761, imParaProcesar.rows * 0.827451 ) ) );

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
