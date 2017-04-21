#ifndef PUPILDETECTIONMETHOD_H
#define PUPILDETECTIONMETHOD_H

#include <string>

#include <opencv2/core.hpp>

class PupilDetectionMethod
{
public:
    PupilDetectionMethod() {}
    ~PupilDetectionMethod() {}

    virtual cv::RotatedRect run(const cv::Mat &frame) = 0;
    virtual bool hasPupilOutline() = 0;
    std::string description() { return mDesc; }

protected:
    std::string mDesc;
};

#endif // PUPILDETECTIONMETHOD_H
