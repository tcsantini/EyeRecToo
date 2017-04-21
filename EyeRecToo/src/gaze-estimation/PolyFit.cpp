#include "PolyFit.h"
#include <QDebug>

using namespace std;
using namespace cv;

PolyFit::PolyFit(PlType plType)
{
    this->plType = plType;

    switch (plType) {
        case POLY_1_X_Y_XY:
            unknowns = 4;
            break;
        case POLY_1_X_Y_XY_XX_YY:
            unknowns = 6;
            break;
        case POLY_1_X_Y_XY_XX_YY_XXYY:
            unknowns = 7;
            break;
        case POLY_1_X_Y_XY_XX_YY_XYY_YXX:
            unknowns = 8;
            break;
        case POLY_1_X_Y_XY_XX_YY_XYY_YXX_XXYY:
            unknowns = 9;
            break;
        case POLY_1_X_Y_XY_XX_YY_XYY_YXX_XXX_YYY:
            unknowns = 10;
        default:
            break;
    }
}

string PolyFit::description()
{
    switch (plType) {
        case POLY_1_X_Y_XY:
            return "POLY_X_Y_XY";
        case POLY_1_X_Y_XY_XX_YY:
            return "POLY_X_Y_XY_XX_YY";
        case POLY_1_X_Y_XY_XX_YY_XXYY:
            return "POLY_X_Y_XY_XX_YY_XXYY";
        case POLY_1_X_Y_XY_XX_YY_XYY_YXX:
            return "POLY_X_Y_XY_XX_YY_XYY_YXX";
        case POLY_1_X_Y_XY_XX_YY_XYY_YXX_XXYY:
            return "POLY_X_Y_XY_XX_YY_XYY_YXX_XXYY";
        case POLY_1_X_Y_XY_XX_YY_XYY_YXX_XXX_YYY:
            return "POLY_X_Y_XY_XX_YY_XYY_YXX_XXX_YYY";
        default:
            return "Unknown";
    }
}

PolyFit::~PolyFit()
{

}

bool PolyFit::calibrate(vector<CollectionTuple> &calibrationTuples, InputType inputType, QString &errorMsg)
{
    calibrationInputType = UNKNOWN;

    PointVector leftPupil;
    PointVector rightPupil;
    PointVector meanPupil;
    PointVector gaze;

    for (unsigned int i=0; i<calibrationTuples.size(); i++){
        leftPupil.insert(calibrationTuples[i].lEye.pupil.center);
        rightPupil.insert(calibrationTuples[i].rEye.pupil.center);
        meanPupil.insert((calibrationTuples[i].lEye.pupil.center + calibrationTuples[i].rEye.pupil.center)/2.0);
        gaze.insert(calibrationTuples[i].field.collectionMarker.center);
    }

    if ( gaze.size() < unknowns ) {
        errorMsg = QString("Not enough calibration points (%1/%2).").arg(gaze.size()).arg(unknowns);
        return false;
    }

    switch (inputType) {
        case BINOCULAR:
            if (!calibrate(meanPupil.x, meanPupil.y, gaze.x, mcx, errorMsg))
                return false;
            if (!calibrate(meanPupil.x, meanPupil.y, gaze.y, mcy, errorMsg))
                return false;
            break;

        case UNKNOWN:
            return false;

        default:
            if (!calibrate(leftPupil.x, leftPupil.y, gaze.x, lcx, errorMsg))
                return false;
            if (!calibrate(leftPupil.x, leftPupil.y, gaze.y, lcy, errorMsg))
                return false;
            if (!calibrate(rightPupil.x, rightPupil.y, gaze.x, rcx, errorMsg))
                return false;
            if (!calibrate(rightPupil.x, rightPupil.y, gaze.y, rcy, errorMsg))
                return false;
            break;
    }

    calibrationInputType = inputType;

    buildDepthMap(calibrationTuples);

    return true;
}

Point3f PolyFit::estimateGaze(const CollectionTuple &tuple, const InputType inputType)
{
    Point2f estimate = evaluate(tuple.lEye.pupil.center, tuple.rEye.pupil.center, inputType);

    Point3f estimate3D;
    estimate3D.x = estimate.x;
    estimate3D.y = estimate.y;
    if (estimate.x < 0 || estimate.x >= depthMap.cols || estimate.y < 0 || estimate.y >= depthMap.rows)
        estimate3D.z = depthMap.at<float>(0, 0);
    else
        estimate3D.z = depthMap.at<float>(estimate.y, estimate.x);

    return estimate3D;
}

Point2f PolyFit::evaluate(Point2f leftPupil, Point2f rightPupil, InputType inputType)
{
    Point2f estimate(-1, -1);
    if (inputType != calibrationInputType)
        return estimate;

    switch (calibrationInputType) {
        case BINOCULAR:
            estimate.x = evaluate(
                             (leftPupil.x + rightPupil.x)/2.0,
                             (leftPupil.y + rightPupil.y)/2.0,
                             mcx);
            estimate.y = evaluate(
                             (leftPupil.x + rightPupil.x)/2.0,
                             (leftPupil.y + rightPupil.y)/2.0,
                             mcy);
            break;

        case BINOCULAR_MEAN_POR:
            estimate.x = evaluate(
                             leftPupil.x,
                             leftPupil.y,
                             lcx)
                     + evaluate(
                             rightPupil.x,
                             rightPupil.y,
                             rcx);
            estimate.x /= 2.0;
            estimate.y = evaluate(
                             rightPupil.x,
                             rightPupil.y,
                             rcy)
                        + evaluate(
                             leftPupil.x,
                             leftPupil.y,
                             lcy);
            estimate.y /= 2.0;
            break;

        case MONO_LEFT:
            estimate.x = evaluate(
                             leftPupil.x,
                             leftPupil.y,
                             lcx);
            estimate.y = evaluate(
                             leftPupil.x,
                             leftPupil.y,
                             lcy);
            break;

        case MONO_RIGHT:
            estimate.x = evaluate(
                             rightPupil.x,
                             rightPupil.y,
                             rcx);
            estimate.y = evaluate(
                             rightPupil.x,
                             rightPupil.y,
                             rcy);
            break;

        default:
            break;
    }

    if (calibrationInputType == BINOCULAR_MEAN_POR) {
        Point2f le, re;
        le.x = evaluate(
                       leftPupil.x,
                       leftPupil.y,
                       lcx);
        le.y = evaluate(
                       leftPupil.x,
                       leftPupil.y,
                       lcy);
        re.x = evaluate(
                       rightPupil.x,
                       rightPupil.y,
                       rcx);
        re.y = evaluate(
                       rightPupil.x,
                       rightPupil.y,
                       rcy);
    }

    return estimate;
}

float PolyFit::evaluate(float x, float y, Mat1d &c)
{
    double xx = x*x;
    double yy = y*y;
    double xy = x*y;
    double xyy = x*yy;
    double yxx = y*xx;
    double xxyy = xx*yy;
    double xxx = x*xx;
    double yyy = y*yy;

    float estimate;

    switch (plType) {
        case POLY_1_X_Y_XY:
            estimate = c.at<double>(0) +
                         x*c.at<double>(1) +
                         y*c.at<double>(2) +
                         xy*c.at<double>(3);
            break;

        case POLY_1_X_Y_XY_XX_YY:
            estimate = c.at<double>(0) +
                         x*c.at<double>(1) +
                         y*c.at<double>(2) +
                         xy*c.at<double>(3) +
                         xx*c.at<double>(4) +
                         yy*c.at<double>(5);
            break;


        case POLY_1_X_Y_XY_XX_YY_XXYY:
            estimate = c.at<double>(0) +
                         x*c.at<double>(1) +
                         y*c.at<double>(2) +
                         xy*c.at<double>(3) +
                         xx*c.at<double>(4) +
                         yy*c.at<double>(5) +
                         xxyy*c.at<double>(6);
            break;

        case POLY_1_X_Y_XY_XX_YY_XYY_YXX:
            estimate = c.at<double>(0) +
                         x*c.at<double>(1) +
                         y*c.at<double>(2) +
                         xy*c.at<double>(3) +
                         xx*c.at<double>(4) +
                         yy*c.at<double>(5) +
                         xyy*c.at<double>(6) +
                         yxx*c.at<double>(7);
            break;

        case POLY_1_X_Y_XY_XX_YY_XYY_YXX_XXYY:
            estimate = c.at<double>(0) +
                         x*c.at<double>(1) +
                         y*c.at<double>(2) +
                         xy*c.at<double>(3) +
                         xx*c.at<double>(4) +
                         yy*c.at<double>(5) +
                         xyy*c.at<double>(6) +
                         yxx*c.at<double>(7) +
                         xxyy*c.at<double>(8);
            break;

        case POLY_1_X_Y_XY_XX_YY_XYY_YXX_XXX_YYY:
            estimate = c.at<double>(0) +
                         x*c.at<double>(1) +
                         y*c.at<double>(2) +
                         xy*c.at<double>(3) +
                         xx*c.at<double>(4) +
                         yy*c.at<double>(5) +
                         xyy*c.at<double>(6) +
                         yxx*c.at<double>(7) +
                         xxx*c.at<double>(8) +
                         yyy*c.at<double>(9);
            break;

        default:
            break;
    }

    return estimate;
}

bool PolyFit::calibrate(Mat &x, Mat &y, Mat &z, Mat1d &c, QString &errorMsg)
{
    // Possible terms; not using more than third degree atm
    Mat xx, yy, xy, xyy, yxx, xxyy, xxx, yyy, A;

    multiply(x, x, xx);
    multiply(y, y, yy);
    multiply(x, y, xy);
    multiply(x, yy, xyy);
    multiply(y, xx, yxx);
    multiply(xx, yy, xxyy);
    multiply(x, xx, xxx);
    multiply(y, yy, yyy);
    A = Mat::ones(z.rows, unknowns, CV_64F);

    switch (plType) {
        case POLY_1_X_Y_XY:
            // A.col(0) was initialized with one
            x.copyTo(A.col(1));
            y.copyTo(A.col(2));
            xy.copyTo(A.col(3));
            break;

        case POLY_1_X_Y_XY_XX_YY:
            // A.col(0) was initialized with one
            x.copyTo(A.col(1));
            y.copyTo(A.col(2));
            xy.copyTo(A.col(3));
            xx.copyTo(A.col(4));
            yy.copyTo(A.col(5));
            break;

        case POLY_1_X_Y_XY_XX_YY_XXYY:
            // A.col(0) was initialized with one
            x.copyTo(A.col(1));
            y.copyTo(A.col(2));
            xy.copyTo(A.col(3));
            xx.copyTo(A.col(4));
            yy.copyTo(A.col(5));
            xxyy.copyTo(A.col(6));
            break;

        case POLY_1_X_Y_XY_XX_YY_XYY_YXX:
            // A.col(0) was initialized with one
            x.copyTo(A.col(1));
            y.copyTo(A.col(2));
            xy.copyTo(A.col(3));
            xx.copyTo(A.col(4));
            yy.copyTo(A.col(5));
            xyy.copyTo(A.col(6));
            yxx.copyTo(A.col(7));
            break;

        case POLY_1_X_Y_XY_XX_YY_XYY_YXX_XXYY:
            // A.col(0) was initialized with one
            x.copyTo(A.col(1));
            y.copyTo(A.col(2));
            xy.copyTo(A.col(3));
            xx.copyTo(A.col(4));
            yy.copyTo(A.col(5));
            xyy.copyTo(A.col(6));
            yxx.copyTo(A.col(7));
            xxyy.copyTo(A.col(8));
            break;

        case POLY_1_X_Y_XY_XX_YY_XYY_YXX_XXX_YYY:
            // A.col(0) was initialized with one
            x.copyTo(A.col(1));
            y.copyTo(A.col(2));
            xy.copyTo(A.col(3));
            xx.copyTo(A.col(4));
            yy.copyTo(A.col(5));
            xyy.copyTo(A.col(6));
            yxx.copyTo(A.col(7));
            xxx.copyTo(A.col(8));
            yyy.copyTo(A.col(9));
            break;

        default:
            break;
    }

    z.convertTo(z, A.type());

    bool result = true;
    result &= solve(A, z, c, DECOMP_SVD);

    for (int i=0; i<c.rows; i++)
        if ( std::isnan(c(i)) ) {
            errorMsg = "found NaN coefficients.";
            result = false;
        }

    return result;
}

void PolyFit::buildDepthMap(vector<CollectionTuple> &tuples)
{
    depthMap = Mat(tuples[0].field.height, tuples[0].field.width, CV_32F, -1.0);
    Rect rect(0,0, depthMap.cols, depthMap.rows);

    Subdiv2D subdiv(Rect(0, 0, tuples[0].field.width, tuples[0].field.height));
    for (unsigned int i=0; i<tuples.size(); i++) {
        Point point = to2D(tuples[i].field.collectionMarker.center);
        depthMap.at<float>(point) = tuples[i].field.collectionMarker.center.z;
        subdiv.insert(point);
    }

    vector<Vec6f> triangleList;
    subdiv.getTriangleList(triangleList);
    vector<Point> pt(3);

    for( unsigned int i = 0; i < triangleList.size(); i++ )
    {
        Vec6f t = triangleList[i];
        vector<Point> vertices;
        vertices.push_back(Point(cvRound(t[0]), cvRound(t[1])));
        vertices.push_back(Point(cvRound(t[2]), cvRound(t[3])));
        vertices.push_back(Point(cvRound(t[4]), cvRound(t[5])));
        if ( rect.contains(vertices[0]) && rect.contains(vertices[1]) && rect.contains(vertices[2])) {
            Rect aoi = boundingRect(vertices);
            float mu = (depthMap.at<float>(vertices[0]) + depthMap.at<float>(vertices[1]) + depthMap.at<float>(vertices[2])) / 3;
            for (int y=aoi.y; y<aoi.y+aoi.height; y++)
                for (int x=aoi.x; x<aoi.x+aoi.width; x++)
                    if ( pointPolygonTest(vertices, Point2f(x,y), false) >= 0)
                        depthMap.at<float>(y,x) = mu;
        }
    }

    int validCount = 0;
    double validAcc = 0;
    for (int y=0; y<depthMap.rows; y++)
        for (int x=0; x<depthMap.cols; x++)
            if (depthMap.at<float>(y,x) >= 0) {
                validCount++;
                validAcc += depthMap.at<float>(y,x);
            }

    if (true){  // use depthMap interpolation
        Mat mask;
        threshold(depthMap, mask, -1, 255, cv::THRESH_BINARY_INV);
        mask.convertTo(mask, CV_8U);
        depthMap.setTo(validAcc/validCount, mask);
    } else // use mean value
        depthMap.setTo(validAcc/validCount);
}
