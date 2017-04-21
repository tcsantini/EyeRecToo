#include "FieldImageProcessor.h"
#include <opencv2/highgui.hpp>

using namespace std;
using namespace cv;
using namespace aruco;

static int gFieldDataId = qRegisterMetaType<FieldData>("FieldData");

FieldImageProcessor::FieldImageProcessor(QString id, QObject *parent)
    : id(id),
      sROI(QPointF(0,0)),
      eROI(QPointF(1,1)),
      forceSanitize(false),
      QObject(parent)
{
    settings = new QSettings(gCfgDir + "/" + id + " ImageProcessor", QSettings::IniFormat);
    updateConfig();

    dict = getPredefinedDictionary(DICT_4X4_250);
    detectorParameters = new DetectorParameters();
    detectorParameters->markerBorderBits = 2;
    detectorParameters->minMarkerPerimeterRate = 0.10; // TODO: determine a good value for these based on the fov and maximum detection distance
    //printMarkers(); // TODO: parametrize me
}

void FieldImageProcessor::updateConfig()
{
    QMutexLocker locker(&cfgMutex);
    cfg.load(settings);
    forceSanitize = true;
}

FieldImageProcessor::~FieldImageProcessor()
{
    if(settings)
        settings->deleteLater();
}

void FieldImageProcessor::process(Timestamp timestamp, const Mat &frame)
{
    Mat prevInput = data.input;

    // TODO: parametrize frame drop due to lack of processing power
    if (gTimer.elapsed() - timestamp > 50)
        return;

    QMutexLocker locker(&cfgMutex);

    data.timestamp = timestamp;

    // Always force the creation of a new matrix for the input since the old one might still be alive from further down the pipeline
    if (cfg.inputSize.width > 0 && cfg.inputSize.height > 0) {
        data.input = Mat(cfg.inputSize, frame.type() );
        resize(frame, data.input, cfg.inputSize);
    }
    else
        data.input = frame.clone();

    if (cfg.flip != CV_FLIP_NONE)
        flip(data.input, data.input, cfg.flip);

    sanitizeCameraParameters( Size(data.input.cols, data.input.rows) );

    data.undistorted = cfg.undistort;
    if (data.undistorted)
        remap(data.input, data.input, map1, map2, CV_INTER_AREA);

    data.width = data.input.cols;
    data.height = data.input.rows;

    // Marker detection and pose estimation
    vector<int> ids;
    vector<vector<Point2f> > corners;
    if (cfg.processingDownscalingFactor > 1) {
        Mat downscaled;
        resize(data.input, downscaled, Size(),
               1/cfg.processingDownscalingFactor,
               1/cfg.processingDownscalingFactor,
               INTER_AREA);
        if (cfg.markerDetectionMethod == "aruco") {
            detectMarkers(downscaled, dict, corners, ids, detectorParameters);
            for (unsigned int i=0; i<ids.size(); i++)
                for (unsigned int j=0; j<corners[i].size(); j++)
                    corners[i][j] = cfg.processingDownscalingFactor*corners[i][j];
        }
    } else
        if (cfg.markerDetectionMethod == "aruco")
            detectMarkers(data.input, dict, corners, ids, detectorParameters);

    // Filling the marker data
    data.collectionMarker = Marker();
    data.markers.clear();

    // Note that the following is based on the COLLECTION MARKER size.
    if (ids.size() > 0) {
        if (data.undistorted) {
            Mat emptyCameraMatrix = (Mat_<double>(3,3) <<
                           data.width,          0, data.width/2,
                           0         , data.width, data.height/2,
                           0         ,0          , 1);
            Mat emptyDistCoeffs = (Mat_<double>(1,4) << 0, 0, 0, 0);
            estimatePoseSingleMarkers(corners, cfg.collectionMarkerSizeMeters, emptyCameraMatrix, emptyDistCoeffs, rvecs, tvecs);
        } else
            estimatePoseSingleMarkers(corners, cfg.collectionMarkerSizeMeters, cameraMatrix, distCoeffs, rvecs, tvecs);
    }

    for (unsigned int i=0; i<ids.size(); i++) {
        Marker marker(corners[i], ids[i]);

        marker.center = estimateMarkerCenter(marker.corners);
        marker.center.z = tvecs.at<double>(i,2);
        marker.tv = ( Mat_<float>(1,3) << tvecs.at<double>(i,0), tvecs.at<double>(i,1), tvecs.at<double>(i,2) );
        marker.rv = ( Mat_<float>(1,3) << rvecs.at<double>(i,0), rvecs.at<double>(i,1), rvecs.at<double>(i,2) );

        data.markers.push_back( marker );

        // use closest calibration marker -- to try and avoid detecting the one viewed in the field camera when testing :-)
        if (marker.id == cfg.collectionMarkerId) {
            if (data.collectionMarker.id == -1)
                data.collectionMarker = data.markers.back();
            else if (data.collectionMarker.center.z > marker.center.z)
                data.collectionMarker = data.markers.back();
        }
    }

    data.validGazeEstimate = false;
    data.processingTimestamp = gTimer.elapsed();

    Q_ASSERT_X(prevInput.data != data.input.data, "Field Image Processing", "Previous and current input image matches!");
    emit newData(data);
}

void FieldImageProcessor::newROI(QPointF sROI, QPointF eROI)
{
    QMutexLocker locker(&cfgMutex);
    if (sROI.isNull() || eROI.isNull()) {
        this->sROI = QPointF(0,0);
        this->eROI = QPointF(1,1);
    } else {
        this->sROI = sROI;
        this->eROI = eROI;
    }
}

void FieldImageProcessor::sanitizeCameraParameters(Size size)
{
    if (!forceSanitize) {
        if ( (size == imageSize) && !map1.empty() && !map2.empty() )
            return;
    } else
        forceSanitize = false;

    FileStorage fs( QString(gCfgDir + "/" + id + "Calibration.xml").toStdString(), FileStorage::READ);
    fs["cameraMatrix"] >> cameraMatrix;
    fs["distCoeffs"] >> distCoeffs;
    fs["imageSize"] >> imageSize;
    bool haveCameraParameters = false;
    if (!cameraMatrix.empty() && !distCoeffs.empty())
        haveCameraParameters = true;

    double currentAspectRatio = size.width / (float) size.height;
    double originalAspectRatio = imageSize.width / (float) imageSize.height;

    if (haveCameraParameters && (currentAspectRatio == originalAspectRatio) ) {
        qInfo() << "Found intrinsic parameters.";
        double rx = imageSize.width  / (double) size.width;
        double ry = imageSize.height / (double) size.height;
        cameraMatrix.at<double>(0,2) = cameraMatrix.at<double>(0,2) / rx;
        cameraMatrix.at<double>(1,2) = cameraMatrix.at<double>(1,2) / ry;
        cameraMatrix.at<double>(0,0) = cameraMatrix.at<double>(0,0) / rx;
        cameraMatrix.at<double>(1,1) = cameraMatrix.at<double>(1,1) / rx;
        imageSize = size;
    } else {
        qInfo() << "No valid intrinsic parameters available. Using dummy values";
        // Dummy estimation
        cameraMatrix = (Mat_<double>(3,3) <<
                        size.width, 0, size.width,
                        0, size.width, size.height,
                        0, 0 ,1 );
        distCoeffs = (Mat_<double>(1,4) << 0, 0, 0, 0);
        imageSize = size;
    }

    initUndistortRectifyMap(
                            cameraMatrix,
                            distCoeffs,
                            Mat(),
                            getOptimalNewCameraMatrix(cameraMatrix, distCoeffs, imageSize, 0, imageSize),
                            imageSize,
                            CV_32FC1,
                            map1,
                            map2
                            );
}
