//#ifndef VISOR_H
//#define VISOR_H

//#define RESOLUTION_WIDTH  640
//#define RESOLUTION_HEIGHT 480

//#include <stdio.h>
//#include <stdlib.h>

//#include <QDir>
//#include <QFile>
//#include <QTimer>
//#include <QLabel>
//#include <QVector>
//#include <QKeyEvent>
//#include <QMouseEvent>

//#include <opencv/highgui.h>
//#include <opencv2/opencv.hpp>
//#include <opencv2/highgui/highgui.hpp>
//#include <opencv2/imgproc/imgproc.hpp>
//#include <opencv2/imgcodecs.hpp>

//using namespace cv;

//class Visor : public QLabel  {
//    Q_OBJECT

//private:
//    VideoCapture *videoCapture;
//    QTimer *sceneTimer;
//    cv::Mat frame;

//    /**
//     * @brief camaraActiva Para que videoCapture->operator >>( frame ); se use o no segun esta o no encendida la camara
//     */
//    bool camaraActiva;

//public:
//    Visor( QWidget *parent = 0 );
//    ~Visor();

//    const cv::Mat& getFrame();

//    void iniciarCamara( int msTimer, int nroCamara = 0 );
//    void setMat( const Mat &im );
//    void setImagenDisco( QString archivo );

//    bool getCamaraActiva() const;

//private slots:
//    void slot_procesar();

//};

//#endif // VISOR_H
