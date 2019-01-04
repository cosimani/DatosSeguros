#include "testeador.h"
#include <QDebug>
#include <QDateTime>

#include "procesador.h"
#include "validador.h"

Testeador::Testeador( QObject * parent ) : QObject( parent ),
                                           decoder( new QZXing )
{

    decoder->setDecoder( QZXing::DecoderFormat_PDF_417 );
}

Testeador::~Testeador()
{
}



void Testeador::procesar( QString directorio )  {

    QDir dir( directorio );
    QStringList fileFilter;
    fileFilter << "*.jpg";
    QStringList imagenes = dir.entryList( fileFilter );

    for ( int i = 0 ; i < imagenes.size() ; i++ )  {
        qDebug() << directorio + "/" +imagenes.at( i );
    }

    for ( int i = 0 ; i < imagenes.size() ; i++ )  {

        qDebug() << "\n Analizando " << imagenes.at( i ) << "\n";

        QImage im( directorio + "/" + imagenes.at( i ) );

        im = im.convertToFormat( QImage::Format_RGB888 );

        cv::Mat imParaAlinear( im.height(), im.width(), CV_8UC3, ( void * )im.constBits(), im.bytesPerLine() );

        cv::cvtColor( imParaAlinear, imParaAlinear, cv::COLOR_RGB2BGR );

        QString fecha_hora = QDateTime::currentDateTime().toString( "yyyyMMddhhmmss" );

        QString paraAlinear = "imagenes/registros/" + fecha_hora + "_paraAlinear.jpg";
        cv::imwrite( paraAlinear.toStdString().c_str(), imParaAlinear );


        /////////////////////////////////////////////////////////////
        /// https://stackoverflow.com/questions/24341114/simple-illumination-correction-in-images-opencv-c
        /// simple illumination correction in images openCV c++
        /////////////////////////////////////////////////////////////

        // READ RGB color image and convert it to Lab
        cv::Mat bgr_image = imParaAlinear.clone();
        cv::Mat lab_image;
        cv::cvtColor( bgr_image, lab_image, CV_BGR2Lab );

        // Extract the L channel
        std::vector< cv::Mat > lab_planes( 3 );
        cv::split( lab_image, lab_planes );  // now we have the L image in lab_planes[0]

        // apply the CLAHE algorithm to the L channel
        cv::Ptr< cv::CLAHE > clahe = cv::createCLAHE();
        clahe->setClipLimit( 4 );
        cv::Mat dst;
        clahe->apply( lab_planes[ 0 ], dst );

        // Merge the the color planes back into an Lab image
        dst.copyTo( lab_planes[ 0 ] );
        cv::merge( lab_planes, lab_image );

       // convert back to RGB
       cv::Mat image_clahe;
       cv::cvtColor( lab_image, image_clahe, CV_Lab2BGR );

       // display the results  (you might also want to see lab_planes[0] before and after).
    //               cv::imshow("image original", bgr_image);
    //               cv::imshow("image CLAHE", image_clahe);


       QString paraclahe = "imagenes/registros/" + fecha_hora + "_clahe.jpg";
       cv::imwrite( paraclahe.toStdString().c_str(), image_clahe );


        QString resultadoZXing;
        cv::Mat matParaLeerCodigo;


        for ( int blockSize = 3 ; blockSize < 105 ; blockSize+=2 )  {
//            for ( int C = 0 ; C < 105 ; C++ )  {

                cv::cvtColor( imParaAlinear, matParaLeerCodigo, CV_RGB2GRAY );

        //            cv::threshold( matParaLeerCodigo, matParaLeerCodigo, umbral, 255, cv::THRESH_BINARY );

        //            // Erode the image with 3x3 kernel
        //            cv::erode( matParaLeerCodigo, matParaLeerCodigo,
        //                       cv::getStructuringElement( cv::MORPH_RECT, cv::Size( 3, 3 ) ) );

                cv::adaptiveThreshold( matParaLeerCodigo, matParaLeerCodigo, 255,
                                       CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY, blockSize, 15 );

                QString paraLeerCodigo1 = "imagenes/registros/" + fecha_hora + "_paraLeerCodigo1_" +
                                           QString::number( blockSize ) + ".jpg";
                cv::imwrite( paraLeerCodigo1.toStdString().c_str(), matParaLeerCodigo );

//            }
        }




        cv::Mat imAlineada;

        // Busca el codigo antes de alinear
        resultadoZXing = decoder->decodeImage( QImage( matParaLeerCodigo.data, matParaLeerCodigo.cols,
                                                       matParaLeerCodigo.rows, matParaLeerCodigo.step,
                                                       QImage::Format_Grayscale8 ) );

        Procesador::getInstancia()->alinear( imParaAlinear, imAlineada, Procesador::DNI );

        if ( imAlineada.rows == 0 )  {
            qDebug() << "{ \"error\":\"Parece que no hay un DNI en la imagen.\" }";

            return;
        }

        if ( resultadoZXing.isEmpty() )  {
            cv::cvtColor( imAlineada, matParaLeerCodigo, CV_RGB2GRAY );
    //                cv::threshold( matParaLeerCodigo, matParaLeerCodigo, umbral, 255, cv::THRESH_BINARY );

            cv::adaptiveThreshold( matParaLeerCodigo, matParaLeerCodigo, 255,
                                   CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY, 91, 15 );

            QString paraLeerCodigo2 = "imagenes/registros/" + fecha_hora + "_paraLeerCodigo2.jpg";
            cv::imwrite( paraLeerCodigo2.toStdString().c_str(), matParaLeerCodigo );
            resultadoZXing = decoder->decodeImage( QImage( matParaLeerCodigo.data,
                                                           matParaLeerCodigo.cols,
                                                           matParaLeerCodigo.rows,
                                                           matParaLeerCodigo.step,
                                                           QImage::Format_Grayscale8 ) );
        }

        QString alineada = "imagenes/registros/" + fecha_hora + "_alineada.jpg";
        cv::imwrite( alineada.toStdString().c_str(), imAlineada );

        cv::Mat imConRectangulos;

        QStringList datosExtraidos = Procesador::getInstancia()->extraerInfo( imAlineada,
                                                                              imConRectangulos,
                                                                              Procesador::DNI );

        QString conRectangulos = "imagenes/registros/" + fecha_hora + "_conRectangulos.jpg";
        cv::imwrite( conRectangulos.toStdString().c_str(), imConRectangulos );


        cv::Mat imFotoPerfil;

        Procesador::getInstancia()->extraerFoto( imAlineada, imFotoPerfil, Procesador::DNI );

        QString fotoPerfil = "imagenes/registros/" + fecha_hora + "_fotoPerfil.jpg";
        cv::imwrite( fotoPerfil.toStdString().c_str(), imFotoPerfil );

        QString json;

        // Este if es para cuando no se pudo con el codigo, entocnes devuelve los datos de la imagen
        if ( resultadoZXing.isEmpty() )  {
            json = Validador::getInstance()->validarDniDeImagen( datosExtraidos );
        }
        else  {
            json = Validador::getInstance()->validaDniDeCodigo( resultadoZXing.split( "@" ) );
        }

        qDebug() << json;



    }



}

using namespace cv;
using namespace std;

void Testeador::procesar2(QString directorio)
{

    QDir dir( directorio );
    QStringList fileFilter;
    fileFilter << "*.jpg";
    QStringList imagenes = dir.entryList( fileFilter );

    for ( int i = 0 ; i < imagenes.size() ; i++ )  {

        QString archivo = directorio + "/" + imagenes.at( i );
        cv::Mat src=imread( archivo.toStdString().c_str() );
        cv::Mat thr;
        cvtColor(src,thr,CV_BGR2GRAY);
        threshold( thr, thr, 70, 255,CV_THRESH_BINARY );

        vector< vector <Point> > contours; // Vector for storing contour
        vector< Vec4i > hierarchy;
        int largest_contour_index=0;
        int largest_area=0;

        Mat dst(src.rows,src.cols,CV_8UC1,Scalar::all(0)); //create destination image
        findContours( thr.clone(), contours, hierarchy,CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE ); // Find the contours in the image
        for( unsigned int i = 0; i< contours.size(); i++ ){
            double a=contourArea( contours[i],false);  //  Find the area of contour
            if(a>largest_area){
                largest_area=a;
                largest_contour_index=i;                //Store the index of largest contour
            }
        }

        drawContours( dst, contours, largest_contour_index, Scalar(255,255,255),CV_FILLED, 8, hierarchy );
        vector<vector<Point> > contours_poly(1);
        approxPolyDP( Mat(contours[largest_contour_index]), contours_poly[0],5, true );

        Rect boundRect=boundingRect(contours[largest_contour_index]);
        if(contours_poly[0].size()==4){
            std::vector<Point2f> quad_pts;
            std::vector<Point2f> squre_pts;
            quad_pts.push_back(Point2f(contours_poly[0][0].x,contours_poly[0][0].y));
            quad_pts.push_back(Point2f(contours_poly[0][1].x,contours_poly[0][1].y));
            quad_pts.push_back(Point2f(contours_poly[0][3].x,contours_poly[0][3].y));
            quad_pts.push_back(Point2f(contours_poly[0][2].x,contours_poly[0][2].y));
            squre_pts.push_back(Point2f(boundRect.x,boundRect.y));
            squre_pts.push_back(Point2f(boundRect.x,boundRect.y+boundRect.height));
            squre_pts.push_back(Point2f(boundRect.x+boundRect.width,boundRect.y));
            squre_pts.push_back(Point2f(boundRect.x+boundRect.width,boundRect.y+boundRect.height));

            Mat transmtx = getPerspectiveTransform(quad_pts,squre_pts);
            Mat transformed = Mat::zeros(src.rows, src.cols, CV_8UC3);
            warpPerspective(src, transformed, transmtx, src.size());
            Point P1=contours_poly[0][0];
            Point P2=contours_poly[0][1];
            Point P3=contours_poly[0][2];
            Point P4=contours_poly[0][3];

            line(src,P1,P2, Scalar(0,0,255),1,CV_AA,0);
            line(src,P2,P3, Scalar(0,0,255),1,CV_AA,0);
            line(src,P3,P4, Scalar(0,0,255),1,CV_AA,0);
            line(src,P4,P1, Scalar(0,0,255),1,CV_AA,0);
            rectangle(src,boundRect,Scalar(0,255,0),1,8,0);
            rectangle(transformed,boundRect,Scalar(0,255,0),1,8,0);


            imwrite("result1.jpg",dst);
            imwrite("result2.jpg",src);
            imwrite("result3.jpg",transformed);
        }
        else
            cout<<"Make sure that your are getting 4 corner using approxPolyDP..."<<endl;



    }

}

void Testeador::procesar3(QString directorio)
{
    QDir dir( directorio );
    QStringList fileFilter;
    fileFilter << "*.jpg";
    QStringList imagenes = dir.entryList( fileFilter );

    for ( int i = 0 ; i < imagenes.size() ; i++ )  {

        QString archivo = directorio + "/" + imagenes.at( i );

        Mat input = imread(archivo.toStdString().c_str());
        Mat input_grey;
        cvtColor(input, input_grey, CV_BGR2GRAY);
        Mat threshold1;
        Mat edges;
        blur(input_grey, input_grey, Size(3, 3));
        Canny(input_grey, edges, 30, 100);

        vector<Point> card_corners = getQuadrilateral(edges, input);
        Mat warpedCard(400, 300, CV_8UC3);
        if (card_corners.size() == 4)
        {
            Mat homography = findHomography(card_corners, vector<Point>{Point(warpedCard.cols, 0),
                                                                        Point(warpedCard.cols, warpedCard.rows),
                                                                        Point(0,0) ,
                                                                        Point(0, warpedCard.rows)});

            warpPerspective(input, warpedCard, homography, Size(warpedCard.cols, warpedCard.rows));
        }

        QString nombre1 = "procesar3_" + QString::number( i ) + "warpedCard.jpg";
        QString nombre2 = "procesar3_" + QString::number( i ) + "edges.jpg";
        QString nombre3 = "procesar3_" + QString::number( i ) + "input.jpg";

        imwrite( nombre1.toStdString().c_str(),warpedCard );
        imwrite( nombre2.toStdString().c_str(),edges );
        imwrite( nombre3.toStdString().c_str(),input );
    }
}

void Testeador::procesar4(QString directorio)
{
    QDir dir( directorio );
    QStringList fileFilter;
    fileFilter << "*.jpg";
    QStringList imagenes = dir.entryList( fileFilter );

    for ( int i = 0 ; i < imagenes.size() ; i++ )  {

        QString archivo = directorio + "/" + imagenes.at( i );

        Mat input = imread(archivo.toStdString().c_str());
        Mat gray;
        Mat umbralCanalH;
        Mat imgBlurred;
        Mat imgCanny;
        Mat img_hsv;
        Mat img_lab;
        Mat adaptive;

        cv::cvtColor( input, img_hsv, CV_RGB2HSV );

        Mat hsvchannels[ img_hsv.channels() ];
        cv::split( img_hsv, hsvchannels );
        Mat matCanalH = hsvchannels[ 0 ].clone();

        QString nombreOriginal = "procesar4_" + QString::number( i ) + "_original.jpg";
        imwrite( nombreOriginal.toStdString().c_str(), input );

        QString nombre2 = "procesar4_" + QString::number( i ) + "_hsv.jpg";
        imwrite( nombre2.toStdString().c_str(), img_hsv );

        QString nombreH = "procesar4_" + QString::number( i ) + "_H.jpg";
        imwrite( nombreH.toStdString().c_str(), matCanalH );

        cv::threshold( matCanalH, umbralCanalH, 0, 255, THRESH_BINARY_INV | THRESH_TRIANGLE );

        cv::dilate( matCanalH, matCanalH, cv::getStructuringElement( cv::MORPH_RECT, cv::Size( 133, 133 ) ) );

        QString nombre1 = "procesar4_" + QString::number( i ) + "_umbralCanalH.jpg";
        imwrite( nombre1.toStdString().c_str(), umbralCanalH );



        // Fuente: http://answers.opencv.org/question/74777/how-to-use-approxpolydp-to-close-contours/
//        Mat srcBlur;
//        Mat srcCanny;

//        cv::erode( matCanalH, matCanalH, cv::getStructuringElement( cv::MORPH_RECT, cv::Size( 11, 11 ) ) );
//        cv::dilate( matCanalH, matCanalH, cv::getStructuringElement( cv::MORPH_RECT, cv::Size( 11, 11 ) ) );

//        blur( matCanalH, srcBlur, Size( 3, 3 ) );

//        Canny( srcBlur, srcCanny, 0, 100, 3, true );



//        vector<vector<Point> > contonos;

//        findContours( srcCanny, contonos, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE );

//        Mat drawing = Mat::zeros(srcCanny.size(), CV_8UC3);

//        for ( int i = 0 ; i < contonos.size() ; i++ )  {
//            Scalar color = Scalar( 255, 255, 255 );
//            drawContours( drawing, contonos, i, color, 2 );
//        }

//        vector< Point > ConvexHullPoints;
//        vector< Point > pts;
//        for ( size_t i = 0 ; i < contonos.size() ; i++ )
//            for ( size_t j = 0 ; j < contonos[ i ].size() ; j++ )
//                pts.push_back( contonos[ i ][ j ] );
//        convexHull( pts, ConvexHullPoints );

//        polylines( drawing, ConvexHullPoints, true, Scalar( 0, 0, 255 ), 2 );
//        QString nombredrawing = "procesar4_" + QString::number( i ) + "_drawing.jpg";
//        imwrite( nombredrawing.toStdString().c_str(), drawing );

//        polylines( matCanalH, ConvexHullPoints, true, Scalar( 0, 0, 255 ), 2 );

//        QString nombrematCanalH_contorno = "procesar4_" + QString::number( i ) + "_matCanalH_contorno.jpg";
//        imwrite( nombrematCanalH_contorno.toStdString().c_str(), matCanalH );












//        //Find the contours. Use the contourOutput Mat so the original image doesn't get overwritten
//        std::vector< std::vector< cv::Point > > contours;
//        cv::Mat contourOutput = umbralCanalH.clone();
//        cv::findContours( contourOutput, contours, CV_RETR_LIST, CV_CHAIN_APPROX_NONE );

//        //Draw the contours
//        cv::Mat contourImage( umbralCanalH.size(), CV_8UC3, cv::Scalar( 0, 0, 0 ) );
//        cv::Scalar colors[ 3 ];
//        colors[ 0 ] = cv::Scalar( 255, 0, 0 );
//        colors[ 1 ] = cv::Scalar( 0, 255, 0 );
//        colors[ 2 ] = cv::Scalar( 0, 0, 255 );
//        for ( size_t idx = 0 ; idx < contours.size() ; idx++ ) {
//            cv::drawContours( contourImage, contours, idx, colors[ idx % 3 ] );
//        }

//        QString nombrecontourImage = "procesar4_" + QString::number( i ) + "_contourImage.jpg";
////        QString nombre3 = "procesar4_" + QString::number( i ) + "_imgBlurred.jpg";
////        QString nombre7 = "procesar4_" + QString::number( i ) + "_imgCanny.jpg";
//        imwrite( nombrecontourImage.toStdString().c_str(), contourImage );











//        cv::GaussianBlur( matCanalH,                 // input image
//            imgBlurred,                         // output image
//            cv::Size( 5, 5 ),                   // smoothing window width and height in pixels
//            5 );                                // sigma value, determines how much the image will be blurred

//        cv::Canny( imgBlurred,          // input image
//            imgCanny,                   // output image
//            0,                          // low threshold
//            100 );                      // high threshold

//        QString nombre3 = "procesar4_" + QString::number( i ) + "_imgBlurred.jpg";
//        QString nombre7 = "procesar4_" + QString::number( i ) + "_imgCanny.jpg";
//        imwrite( nombre7.toStdString().c_str(), imgBlurred );
//        imwrite( nombre3.toStdString().c_str(), imgCanny );

//        cv::cvtColor( input, img_lab, CV_RGB2Lab );

//        cv::adaptiveThreshold( rgbchannel[ 0 ], adaptive, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 115, 1 );
//        QString nombre4 = "procesar4_" + QString::number( i ) + "_adaptive.jpg";
//        imwrite( nombre4.toStdString().c_str(), adaptive );







        // define range of white color in HSV
        // change it according to your need !
//        lower_white = np.array([0,0,0], dtype=np.uint8)
//        upper_white = np.array([0,0,255], dtype=np.uint8)

        // Threshold the HSV image to get only white colors
//        mask = cv2.inRange( hsv, lower_white, upper_white )
        // Bitwise-AND mask and original image
//        res = cv2.bitwise_and(frame,frame, mask= mask)


//        cv::cvtColor( input, gray, CV_BGR2GRAY );       // convert to grayscale





//        cv::cvtColor( img_hsv, gray, CV_BGR2GRAY );       // convert to grayscale



    }


}

using namespace cv;
using namespace cv::xfeatures2d;

void Testeador::procesar5(QString directorio)
{
    QDir dir( directorio );
    QStringList fileFilter;
    fileFilter << "*.jpg";
    QStringList imagenes = dir.entryList( fileFilter );

    Mat img_object = imread( "/home/cosimani/Proyecto/DatosSeguros/GitHub/testing/referencias/licencia-frente.jpg",
                             IMREAD_GRAYSCALE );

    for ( int i = 0 ; i < imagenes.size() ; i++ )  {

        QString archivo = directorio + "/" + imagenes.at( i );

        Mat img_scene = imread( archivo.toStdString().c_str(), IMREAD_GRAYSCALE );

        //-- Step 1: Detect the keypoints using SURF Detector, compute the descriptors
        int minHessian = 400;
        Ptr< SURF > detector = SURF::create( minHessian );
        std::vector<KeyPoint> keypoints_object, keypoints_scene;
        Mat descriptors_object, descriptors_scene;
        detector->detectAndCompute( img_object, noArray(), keypoints_object, descriptors_object );
        detector->detectAndCompute( img_scene, noArray(), keypoints_scene, descriptors_scene );

        //-- Step 2: Matching descriptor vectors with a FLANN based matcher
        // Since SURF is a floating-point descriptor NORM_L2 is used
        Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create(DescriptorMatcher::FLANNBASED);
        std::vector< std::vector<DMatch> > knn_matches;
        matcher->knnMatch( descriptors_object, descriptors_scene, knn_matches, 2 );

        //-- Filter matches using the Lowe's ratio test
        const float ratio_thresh = 0.75f;

        std::vector< DMatch > good_matches;

        for (size_t i = 0; i < knn_matches.size(); i++)
        {
            if ( knn_matches[ i ][ 0 ].distance < ratio_thresh * knn_matches[ i ][ 1 ].distance )
            {
                good_matches.push_back( knn_matches[ i ][ 0 ] );
            }
        }

//        if ( good_matches.size() < 500 )
//            continue;

        qDebug() << i << good_matches.size();

        //-- Draw matches
        Mat img_matches;
        drawMatches( img_object, keypoints_object, img_scene, keypoints_scene, good_matches, img_matches, Scalar::all(-1),
                     Scalar::all(-1), std::vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS );

        //-- Localize the object
        std::vector<Point2f> obj;
        std::vector<Point2f> scene;
        for( size_t i = 0; i < good_matches.size(); i++ )
        {
            //-- Get the keypoints from the good matches
            obj.push_back( keypoints_object[ good_matches[i].queryIdx ].pt );
            scene.push_back( keypoints_scene[ good_matches[i].trainIdx ].pt );
        }
        Mat H = findHomography( obj, scene, RANSAC );

        //-- Get the corners from the image_1 ( the object to be "detected" )
        std::vector<Point2f> obj_corners(4);
        obj_corners[0] = Point2f(0, 0);
        obj_corners[1] = Point2f( (float)img_object.cols, 0 );
        obj_corners[2] = Point2f( (float)img_object.cols, (float)img_object.rows );
        obj_corners[3] = Point2f( 0, (float)img_object.rows );
        std::vector<Point2f> scene_corners(4);
        perspectiveTransform( obj_corners, scene_corners, H);

        //-- Draw lines between the corners (the mapped object in the scene - image_2 )
        line( img_matches, scene_corners[0] + Point2f((float)img_object.cols, 0),
              scene_corners[1] + Point2f((float)img_object.cols, 0), Scalar(0, 255, 0), 4 );
        line( img_matches, scene_corners[1] + Point2f((float)img_object.cols, 0),
              scene_corners[2] + Point2f((float)img_object.cols, 0), Scalar( 0, 255, 0), 4 );
        line( img_matches, scene_corners[2] + Point2f((float)img_object.cols, 0),
              scene_corners[3] + Point2f((float)img_object.cols, 0), Scalar( 0, 255, 0), 4 );
        line( img_matches, scene_corners[3] + Point2f((float)img_object.cols, 0),
              scene_corners[0] + Point2f((float)img_object.cols, 0), Scalar( 0, 255, 0), 4 );

        //-- Show detected matches
        QString nombreH = "procesar4_" + QString::number( i ) + "_img_matches.jpg";
        imwrite( nombreH.toStdString().c_str(), img_matches );

    }
}

void Testeador::procesarDniDorso(QString directorio)
{
    QDir dir( directorio );
    QStringList fileFilter;
    fileFilter << "*.jpg";
    QStringList imagenes = dir.entryList( fileFilter );
    qDebug() << imagenes;


    for ( int i = 0 ; i < imagenes.size() ; i++ )  {

        QString archivo = directorio + "/" + imagenes.at( i );

//        Mat imParaAlinear = imread( archivo.toStdString().c_str(), IMREAD_GRAYSCALE );
        Mat imParaAlinear = imread( archivo.toStdString().c_str(), IMREAD_COLOR );

        cv::Mat imAlineada;

        bool exito = Procesador::getInstancia()->alinearSurfFlann( imParaAlinear,
                                                                   imAlineada, Procesador::DNI_DORSO );

        if ( ! exito )  {
            qDebug() << "{ \"error\":\"No se pudo alinear la imagen (DNI dorso). Intente de nuevo!\" }";

            return;
        }

        QString alineada = "imagenes/registros/" + imagenes.at( i ) + "_alineada.jpg";
        cv::imwrite( alineada.toStdString().c_str(), imAlineada );

        cv::Mat imConRectangulos;

        QStringList datosExtraidos = Procesador::getInstancia()->extraerInfo( imAlineada,
                                                                              imConRectangulos,
                                                                              Procesador::DNI_DORSO );

        QString conRectangulos = "imagenes/registros/" + imagenes.at( i ) + "_conRectangulos.jpg";
        cv::imwrite( conRectangulos.toStdString().c_str(), imConRectangulos );

        QString json = Validador::getInstance()->validaDniDorso( datosExtraidos );

        qDebug() << json;

        if ( json.isEmpty() )  {  // Si el json esta vacio es porque no fueron validos los datos
            qDebug() << "{ \"error\":\"No se pudieron extraer correctamente los datos.\" }";
        }
        else  {
            qDebug() << json.toStdString().c_str();
        }

    }
}



Vec3f Testeador::calcParams(Point2f p1, Point2f p2) // line's equation Params computation
{
    float a, b, c;
    if (p2.y - p1.y == 0)
    {
        a = 0.0f;
        b = -1.0f;
    }
    else if (p2.x - p1.x == 0)
    {
        a = -1.0f;
        b = 0.0f;
    }
    else
    {
        a = (p2.y - p1.y) / (p2.x - p1.x);
        b = -1.0f;
    }

    c = (-a * p1.x) - b * p1.y;
    return(Vec3f(a, b, c));
}

Point Testeador::findIntersection(Vec3f params1, Vec3f params2)
{
    float x = -1, y = -1;
    float det = params1[0] * params2[1] - params2[0] * params1[1];
    if (det < 0.5f && det > -0.5f) // lines are approximately parallel
    {
        return(Point(-1, -1));
    }
    else
    {
        x = (params2[1] * -params1[2] - params1[1] * -params2[2]) / det;
        y = (params1[0] * -params2[2] - params2[0] * -params1[2]) / det;
    }
    return(Point(x, y));
}

vector<Point> Testeador::getQuadrilateral(Mat & grayscale, Mat& output) // returns that 4 intersection points of the card
{
    Mat convexHull_mask(grayscale.rows, grayscale.cols, CV_8UC1);
    convexHull_mask = Scalar(0);

    vector<vector<Point>> contours;
    findContours(grayscale, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);

    vector<int> indices(contours.size());
    iota(indices.begin(), indices.end(), 0);

    sort(indices.begin(), indices.end(), [&contours](int lhs, int rhs) {
        return contours[lhs].size() > contours[rhs].size();
    });

    /// Find the convex hull object
    vector<vector<Point> >hull(1);
    convexHull(Mat(contours[indices[0]]), hull[0], false);

    vector<Vec4i> lines;
    drawContours(convexHull_mask, hull, 0, Scalar(255));

    imwrite("procesar3_convexHull_mask.jpg",convexHull_mask);

    HoughLinesP(convexHull_mask, lines, 1, CV_PI / 200, 50, 50, 10);
    std::cout << "lines size:" << lines.size() << endl;

    if (lines.size() == 4) // we found the 4 sides
    {
        vector<Vec3f> params(4);
        for (int l = 0; l < 4; l++)
        {
            params.push_back(calcParams(Point(lines[l][0], lines[l][1]), Point(lines[l][2], lines[l][3])));
        }

        vector<Point> corners;
        for (unsigned int i = 0; i < params.size(); i++)
        {
            for (unsigned int j = i; j < params.size(); j++) // j starts at i so we don't have duplicated points
            {
                Point intersec = findIntersection(params[i], params[j]);
                if ((intersec.x > 0) && (intersec.y > 0) && (intersec.x < grayscale.cols) && (intersec.y < grayscale.rows))
                {
                    cout << "corner: " << intersec << endl;
                    corners.push_back(intersec);
                }
            }
        }

        for (unsigned int i = 0; i < corners.size(); i++)
        {
            circle(output, corners[i], 3, Scalar(0, 0, 255));
        }

        if (corners.size() == 4) // we have the 4 final corners
        {
            return(corners);
        }
    }

    return(vector<Point>());
}
