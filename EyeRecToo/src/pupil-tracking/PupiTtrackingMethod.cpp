#include "PupilTrackingMethod.h"

#include <QtConcurrent/QtConcurrent>
#include <QFuture>

using namespace std;
using namespace cv;

void PupilTrackingMethod::reset()
{
	previousPupils.clear();
	previousPupil = TrackedPupil();
	pupilDiameterKf.statePost.ptr<float>(0)[0] = 0.5*expectedFrameSize.width;
}

void PupilTrackingMethod::registerPupil( const Timestamp &ts, Pupil &pupil ) {
	Mat measurement = ( Mat_<float>(1,1) << pupil.majorAxis() );
	//if (predictedMaxPupilDiameter > 0) {
	//	float &majorAxis = measurement.ptr<float>(0)[0];
	//	if ( majorAxis > predictedMaxPupilDiameter) {
	//		pupil.clear();
	//		return;
	//	}
	//}

	if (pupil.confidence > minDetectionConfidence) {
		previousPupil = TrackedPupil(ts, pupil);
		previousPupils.emplace_back( previousPupil );
		pupilDiameterKf.correct(measurement);
	} else
		previousPupil = TrackedPupil();

	//if (pupil.confidence > minDetectionConfidence) {
	//	previousPupil = TrackedPupil(ts, pupil);
	//	previousPupils.push_back( previousPupil );
	//} else
	//	previousPupil = TrackedPupil();
}

void PupilTrackingMethod::run(const Timestamp &ts, const cv::Mat &frame, const cv::Rect &roi, Pupil &pupil, PupilDetectionMethod &pupilDetectionMethod)
{
	cv::Size frameSize = { frame.cols, frame.rows };
	if (expectedFrameSize != frameSize ) {
		// Reference frame changed. Let's start over!
		expectedFrameSize = frameSize;
		reset();
	}

	// Remove old samples
	while (!previousPupils.empty()) {
		if (ts - previousPupils.front().ts > maxAge)
			previousPupils.pop_front();
		else
			break;
	}

	pupil.clear();
	predictMaxPupilDiameter();

	if ( previousPupil.confidence == NO_CONFIDENCE ) {
		pupil = pupilDetectionMethod.runWithConfidence(frame, roi, -1, predictedMaxPupilDiameter);
	} else {
		run(frame, roi, previousPupil, pupil);
	}

	registerPupil(ts, pupil);
	return;


	/*
	int minPupilDiameterPx = -1;
	int maxPupilDiameterPx = -1;
	if ( ! previousPupils.empty() ) {
		float meanDiameterPx = std::accumulate( previousPupils.begin(), previousPupils.end(), 0,
			[](int sum, const TrackedPupil &p) { return sum + max<int>( p.size.width, p.size.height ); } ) / (float) previousPupils.size();
		minPupilDiameterPx = 0.75 * meanDiameterPx;
		maxPupilDiameterPx = 1.25 * meanDiameterPx;
	}

	if ( previousPupil.confidence == NO_CONFIDENCE ) {
		pupil = pupilDetectionMethod.runWithConfidence(frame, roi, minPupilDiameterPx, maxPupilDiameterPx);
		if (previousPupils.size() > 15) {
			if (pupil.diameter() <= maxPupilDiameterPx)
				updatePreviousPupil(ts, pupil);
			else
				pupil.clear();
		} else
			updatePreviousPupil(ts, pupil);
		return;
	}

	QFuture<Pupil> future;
	if (ts - lastDetection > maxTrackingWithoutDetectionTime) {
		float previousDiameter = max<float>( previousPupil.size.width, previousPupil.size.height );
		future = QtConcurrent::run(&pupilDetectionMethod, &PupilDetectionMethod::runWithConfidence, frame, roi, 0.9*previousDiameter, 1.1*previousDiameter);
		parallelDetection = true;
	}

	run(frame, roi, previousPupil, pupil);

	if (parallelDetection) {
		parallelDetection = false;
		lastDetection = ts;
		Pupil detectedPupil = future.result();
		if (detectedPupil.confidence > pupil.confidence)
			pupil = detectedPupil;
	} else {
		if ( pupil.confidence < minDetectionConfidence)
			pupil = pupilDetectionMethod.runWithConfidence(frame, roi);
	}

	if ( pupil.confidence > minDetectionConfidence) {
		if (previousPupils.size() > 15) {
			if (pupil.diameter() <= maxPupilDiameterPx)
				updatePreviousPupil(ts, pupil);
			else
				pupil.clear();
		} else
			updatePreviousPupil(ts, pupil);
	}
	*/
}
