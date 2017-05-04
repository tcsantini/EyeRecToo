#ifndef HOMOGRAPHY_H
#define HOMOGRAPHY_H

#include <vector>

#include <QString>

#include "opencv2/core.hpp"
#include "opencv2/calib3d/calib3d.hpp"

#include "GazeEstimationMethod.h"

class Homography : public GazeEstimationMethod
{
public:
    Homography();
    bool calibrate(std::vector<CollectionTuple> &calibrationTuples, InputType inputType, QString &errorMsg);
    cv::Point2f evaluate(cv::Point2f leftPupil, cv::Point2f rightPupil, InputType inputType);
    cv::Point3f estimateGaze(const CollectionTuple &tuple, const InputType inputType);
    bool allowCenterAndScale() { return true; }
    std::string description() { return "Homography"; }

private:
    InputType calibrationInputType;
    cv::Mat mH;
    cv::Mat lH;
    cv::Mat rH;
    bool binocularCalibrated;
    bool monoLeftCalibrated;
    bool monoRightCalibrated;

    bool calibrate(std::vector<cv::Point2f> pupil, std::vector<cv::Point2f> gaze, cv::Mat &H, QString &errorMsg);
};

#endif // HOMOGRAPHY_H
