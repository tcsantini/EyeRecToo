#ifndef POLYFIT_H
#define POLYFIT_H

#include <iostream>
#include <vector>

#include "opencv2/core.hpp"

#include <QString>

#include "GazeEstimationMethod.h"

class PointVector
{
public:
    void insert(cv::Point2f p) { x.push_back(p.x); y.push_back(p.y); }
    void insert(cv::Point3f p) { x.push_back(p.x); y.push_back(p.y); z.push_back(p.y); }
    unsigned int size() { return x.rows; }
    cv::Mat x, y, z;
};

class PolyFit : public GazeEstimationMethod
{
public:
    // TODO: Implement other polynoms, maybe even make it general form if we have time :-)
    enum PlType {
        POLY_1_X_Y_XY,
        POLY_1_X_Y_XY_XX_YY,
        POLY_1_X_Y_XY_XX_YY_XXYY,
        POLY_1_X_Y_XY_XX_YY_XYY_YXX,
        POLY_1_X_Y_XY_XX_YY_XYY_YXX_XXYY,
        POLY_1_X_Y_XY_XX_YY_XYY_YXX_XXX_YYY
    };
    PolyFit(PlType plType);
    ~PolyFit();

    bool calibrate(std::vector<CollectionTuple> &calibrationTuples, InputType inputType, QString &errorMsg);
    cv::Point2f evaluate(cv::Point2f leftPupil, cv::Point2f rightPupil, InputType inputType);
    cv::Point3f estimateGaze(const CollectionTuple &tuple, const InputType inputType);
    bool allowCenterAndScale() { return true; }
    std::string description();

private:
    PlType plType;
    cv::Mat1d mcx;
    cv::Mat1d mcy;
    cv::Mat1d lcx;
    cv::Mat1d lcy;
    cv::Mat1d rcx;
    cv::Mat1d rcy;

    cv::Mat depthMap;
    unsigned int unknowns;
    InputType calibrationInputType;
    bool binocularCalibrated;
    bool monoLeftCalibrated;
    bool monoRightCalibrated;

    bool calibrate(cv::Mat &x, cv::Mat &y, cv::Mat &z, cv::Mat1d &c, QString &errorMsg);
    float evaluate(float x, float y, cv::Mat1d &c);
    void buildDepthMap(std::vector<CollectionTuple> &tuples);

};

#endif // POLYFIT_H
