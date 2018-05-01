#include "EyeImageProcessor.h"
#include <opencv2/highgui.hpp>

using namespace cv;

static int gEyeDataId = qRegisterMetaType<EyeData>("EyeData");

EyeImageProcessor::EyeImageProcessor(QString id, QObject *parent)
    : id(id),
      pupilDetectionMethod(NULL),
	  pupilTrackingMethod(NULL),
	  sROI(QPointF(0,0)),
      eROI(QPointF(1,1)),
      QObject(parent)
{
	availablePupilDetectionMethods.push_back(new PuRe());
	availablePupilDetectionMethods.push_back(new ElSe());
	availablePupilDetectionMethods.push_back(new ExCuSe());
#ifdef STARBURST
    availablePupilDetectionMethods.push_back(new Starburst());
#endif
#ifdef SWIRSKI
    availablePupilDetectionMethods.push_back(new Swirski());
#endif
    settings = new QSettings(gCfgDir + "/" + id + " ImageProcessor.ini", QSettings::IniFormat);
    updateConfig();

	pmIdx = gPerformanceMonitor.enrol(id, "Image Processor");

	pupilTrackingMethod = new PuReST();
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
    // TODO: parametrize frame drop due to lack of processing power
	if ( gPerformanceMonitor.shouldDrop(pmIdx, gTimer.elapsed() - timestamp, 50) )
		return;

    QMutexLocker locker(&cfgMutex);

    data.timestamp = timestamp;

	Q_ASSERT_X(frame.data != data.input.data, "Eye Image Processing", "Previous and current input image matches!");
	if (cfg.inputSize.width > 0 && cfg.inputSize.height > 0) {
		data.input = Mat(cfg.inputSize, frame.type() );
		resize(frame, data.input, cfg.inputSize);
	}
	else {
		data.input = frame;
	}

	if (cfg.flip != CV_FLIP_NONE)
		flip(data.input, data.input, cfg.flip);

	if (data.input.channels() > 1) // TODO: make it algorithm dependent
		cvtColor(data.input, data.input, CV_BGR2GRAY);

	data.pupil = Pupil();
	data.validPupil = false;
	if (pupilDetectionMethod != NULL)  {
		Rect userROI = Rect(
				Point(sROI.x() * data.input.cols, sROI.y() * data.input.rows),
				Point( eROI.x() * data.input.cols, eROI.y() * data.input.rows)
			);

		float scalingFactor = 1;
		if (cfg.processingDownscalingFactor > 1)
			scalingFactor = 1.0 / cfg.processingDownscalingFactor;

		/*
		 *  From here on, our reference frame is the scaled user ROI
		 */
		Mat downscaled;
		resize(data.input(userROI), downscaled, Size(),
			   scalingFactor, scalingFactor,
			   INTER_AREA);
		Rect coarseROI = {0, 0, downscaled.cols, downscaled.rows };

        // If the user wants a coarse location and the method has none embedded,
        // we further constrain the search using the generic one
        if (!pupilDetectionMethod->hasCoarseLocation() && cfg.coarseDetection) {
            coarseROI = PupilDetectionMethod::coarsePupilDetection( downscaled, 0.5f, 60, 40);
            data.coarseROI = Rect(
                                 userROI.tl() + coarseROI.tl() / scalingFactor,
                                 userROI.tl() + coarseROI.br() / scalingFactor
                            );
        } else
            data.coarseROI = Rect();

        if (cfg.tracking && pupilTrackingMethod) {
			pupilTrackingMethod->run(timestamp, downscaled, coarseROI, data.pupil, *pupilDetectionMethod);
		} else {
			pupilDetectionMethod->run( downscaled, coarseROI, data.pupil );
			// TODO: expose this to the user
			if ( ! pupilDetectionMethod->hasConfidence() )
				data.pupil.confidence = PupilDetectionMethod::outlineContrastConfidence(downscaled, data.pupil);
        }

		if (data.pupil.center.x > 0 && data.pupil.center.y > 0) {
			// Upscale
			data.pupil.resize( 1.0 / scalingFactor );

			// User region shift
			data.pupil.shift( userROI.tl() );
			data.validPupil = true;
		}

	}

    data.processingTimestamp = gTimer.elapsed() - data.timestamp;

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
