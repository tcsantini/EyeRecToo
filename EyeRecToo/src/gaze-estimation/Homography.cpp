#include "Homography.h"

using namespace std;
using namespace cv;

Homography::Homography()
{

}

bool Homography::calibrate(vector<CollectionTuple> &calibrationTuples, InputType inputType, QString &errorMsg)
{
    vector<Point2f> leftPupil;
    vector<Point2f> rightPupil;
    vector<Point2f> meanPupil;
    vector<Point2f> gaze;
    for (unsigned int i=0; i<calibrationTuples.size(); i++){
        leftPupil.push_back(calibrationTuples[i].lEye.pupil.center);
        rightPupil.push_back(calibrationTuples[i].rEye.pupil.center);
        meanPupil.push_back((calibrationTuples[i].lEye.pupil.center + calibrationTuples[i].rEye.pupil.center)/2.0);
        gaze.push_back( to2D(calibrationTuples[i].field.collectionMarker.center) );
    }

    switch (inputType) {
        case BINOCULAR:
            if (!calibrate(meanPupil, gaze, mH, errorMsg))
                return false;
            break;

        case UNKNOWN:
            return false;

        default:
            if (!calibrate(leftPupil, gaze, lH, errorMsg))
                return false;
            if (!calibrate(rightPupil, gaze, rH, errorMsg))
                return false;
            break;
    }

    calibrationInputType = inputType;
    return true;
}

Point3f Homography::estimateGaze(const CollectionTuple &tuple, const InputType inputType)
{
    Point3f estimate(-1,-1,-1);

    if (inputType != calibrationInputType)
        return estimate;

    vector<Point2d> ev;
    vector<Point2d> pupil;

    switch (calibrationInputType) {
        case BINOCULAR:
            pupil.push_back( (tuple.lEye.pupil.center + tuple.rEye.pupil.center)/2 );
            perspectiveTransform(pupil, ev, mH);
            estimate = to3D(ev[0]);
            break;

        case BINOCULAR_MEAN_POR:
            pupil.push_back( tuple.lEye.pupil.center );
            perspectiveTransform(pupil, ev, lH);
            estimate = to3D(ev[0]);
            pupil.clear();
            pupil.push_back( tuple.rEye.pupil.center );
            perspectiveTransform(pupil, ev, rH);
            estimate = (estimate + to3D(ev[0])) / 2.0;
            break;

        case MONO_LEFT:
            pupil.push_back( tuple.lEye.pupil.center );
            perspectiveTransform(pupil, ev, lH);
            estimate = to3D(ev[0]);
            break;

        case MONO_RIGHT:
            pupil.push_back( tuple.rEye.pupil.center );
            perspectiveTransform(pupil, ev, rH);
            estimate = to3D(ev[0]);
            break;

        default:
            break;
    }

    return estimate;
}

Point2f Homography::evaluate(Point2f leftPupil, Point2f rightPupil, InputType inputType)
{
    (void) leftPupil;
    (void) rightPupil;
    (void) inputType;
    return Point2f();
}

bool Homography::calibrate(vector<Point2f> pupil, vector<Point2f> gaze, Mat &H, QString &errorMsg)
{
    bool useRansac = true;

    if (gaze.size() < 4) // TODO: user may not be using median point, add some smarter test here
        errorMsg = QString("Not enough calibration points (%1/4).").arg(gaze.size());

    if (useRansac) {
        H = findHomography( pupil, gaze, RANSAC);
        if ( H.empty() )
            H = findHomography( pupil, gaze);
    }else
        H = findHomography( pupil, gaze);


    if (H.empty()) {
        errorMsg = QString("Could not find perspective transformation.");
        return false;
    }

    return true;
}
