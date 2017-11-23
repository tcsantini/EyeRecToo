#ifndef PUPILTRACKINGMETHOD_H
#define PUPILTRACKINGMETHOD_H

#include <string>
#include <deque>
#include <QFuture>

#include <opencv2/core.hpp>
#include "pupil-detection/PupilDetectionMethod.h"

#include "utils.h"

class TrackedPupil : public Pupil
{
public:
	TrackedPupil(const Timestamp &ts, const Pupil &pupil) :
		Pupil(pupil),
		ts(ts)
	{}

	TrackedPupil() :
		Pupil(),
		ts(0)
	{}

	Timestamp ts;
};

class PupilTrackingMethod
{
public:
	PupilTrackingMethod() {}
	~PupilTrackingMethod() {}

	// Tracking and detection logic
	void run(const Timestamp &ts, const cv::Mat &frame, const cv::Rect &roi, Pupil &pupil, PupilDetectionMethod &pupilDetectionMethod);

	// Tracking implementation
	virtual void run(const cv::Mat &frame, const cv::Rect &roi, const Pupil &previousPupil, Pupil &pupil, const float &minPupilDiameterPx=-1, const float &maxPupilDiameterPx=-1) = 0;

	std::string description() { return mDesc; }

private:

protected:
	std::string mDesc;

	cv::Size expectedFrameSize = {0, 0};
	TrackedPupil previousPupil;
	std::deque<TrackedPupil> previousPupils;

	Timestamp maxAge = 1500;
	Timestamp maxTrackingWithoutDetectionTime = 5000;
	Timestamp lastDetection;
	bool parallelDetection = false;
	float minDetectionConfidence = 0.7f;
	float minTrackConfidence = 0.9f;

	void updatePreviousPupil( const Timestamp &ts, const Pupil &pupil ) {
		if (pupil.confidence > minDetectionConfidence) {
			previousPupil = TrackedPupil(ts, pupil);
			previousPupils.push_back( previousPupil );
		} else
			previousPupil = TrackedPupil();
	}

	void reset();
};

#endif // PUPILTRACKINGMETHOD_H
