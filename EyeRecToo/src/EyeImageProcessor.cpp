#include "EyeImageProcessor.h"
#include <opencv2/highgui.hpp>

using namespace cv;

static int gEyeDataId = qRegisterMetaType<EyeData>("EyeData");

EyeImageProcessor::EyeImageProcessor(QString id, QObject *parent)
    : id(id),
      pupilDetectionMethod(NULL),
      sROI(QPointF(0,0)),
      eROI(QPointF(1,1)),
      QObject(parent)
{
    availablePupilDetectionMethods.push_back(new ElSe());
    availablePupilDetectionMethods.push_back(new ExCuSe());
#ifdef STARBURST
    availablePupilDetectionMethods.push_back(new Starburst());
#endif
#ifdef SWIRSKI
    availablePupilDetectionMethods.push_back(new Swirski());
#endif
    settings = new QSettings(gCfgDir + "/" + id + " ImageProcessor", QSettings::IniFormat);
    updateConfig();
}

void EyeImageProcessor::updateConfig()
{
    QMutexLocker locker(&cfgMutex);
    cfg.load(settings);

    pupilDetectionMethod = NULL;
    for (int i=0; i<availablePupilDetectionMethods.size(); i++)
        if (cfg.pupilDetectionMethod == QString(availablePupilDetectionMethods[i]->description().c_str()) )
            pupilDetectionMethod = availablePupilDetectionMethods[i];
}

EyeImageProcessor::~EyeImageProcessor()
{
    for (int i=0; i<availablePupilDetectionMethods.size(); i++)
        delete availablePupilDetectionMethods[i];
    availablePupilDetectionMethods.clear();

    if(settings)
        settings->deleteLater();
}

void EyeImageProcessor::process(Timestamp timestamp, const Mat &frame)
{
    Mat prevInput = data.input;

    // TODO: parametrize frame drop due to lack of processing power
    if (gTimer.elapsed() - timestamp > 50)
        return;

    QMutexLocker locker(&cfgMutex);

    data.timestamp = timestamp;

    // Always force the creation of a new matrix for the input since the old one might still be alive from further down the pipeline
    if (cfg.inputSize.width > 0 && cfg.inputSize.height > 0) {
        data.input = Mat::zeros(cfg.inputSize, frame.type() );
        resize(frame, data.input, cfg.inputSize);
    }
    else
        data.input = frame.clone();

    if (cfg.flip != CV_FLIP_NONE)
        flip(data.input, data.input, cfg.flip);

    if (data.input.channels() > 1) // TODO: make it algorithm dependent
        cvtColor(data.input, data.input, CV_BGR2GRAY);

    data.pupil = RotatedRect();
    data.validPupil = false;
    if (pupilDetectionMethod != NULL)  {

        Rect tmp = Rect(
                Point(sROI.x() * data.input.cols, sROI.y() * data.input.rows),
                Point( eROI.x() * data.input.cols, eROI.y() * data.input.rows)
            );
        Point start = tmp.tl();
        Point end = tmp.br();

        Mat roi = data.input(Rect(start, end));

        if (cfg.processingDownscalingFactor > 1) {
            resize(roi, roi, Size(),
                   1/cfg.processingDownscalingFactor,
                   1/cfg.processingDownscalingFactor,
                   INTER_AREA);
        }

        if (roi.rows > 10 && roi.cols > 10) { // minimum size otherwise some algorithms might crash
            if (roi.rows <= 640 && roi.cols <= 640) // TODO: fix ExCuSe and ElSe size limit
                data.pupil = pupilDetectionMethod->run(roi);
        }

        if (data.pupil.center.x > 0 && data.pupil.center.y > 0) {
            if (cfg.processingDownscalingFactor > 1) {
                data.pupil.center.x *= cfg.processingDownscalingFactor;
                data.pupil.center.y *= cfg.processingDownscalingFactor;
                data.pupil.size.width *= cfg.processingDownscalingFactor;
                data.pupil.size.height *= cfg.processingDownscalingFactor;
            }
            data.pupil.center.x += start.x;
            data.pupil.center.y += start.y;
            data.validPupil = true;
        }
    }

    data.processingTimestamp = gTimer.elapsed();

    Q_ASSERT_X(prevInput.data != data.input.data, "Eye Image Processing", "Previous and current input image matches!");
    emit newData(data);
}

void EyeImageProcessor::newROI(QPointF sROI, QPointF eROI)
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
