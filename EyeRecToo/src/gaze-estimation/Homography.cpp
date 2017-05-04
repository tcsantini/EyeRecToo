#include "Homography.h"

using namespace std;
using namespace cv;

Homography::Homography() :
    binocularCalibrated(false),
    monoLeftCalibrated(false),
    monoRightCalibrated(false)
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

    // Estimate parameters for all, independently of input type; this allows us
    // to use a single eye later even if mean pupil was selected for calibration

    binocularCalibrated = calibrate(meanPupil, gaze, mH, errorMsg);
    monoLeftCalibrated = calibrate(leftPupil, gaze, lH, errorMsg);
    monoRightCalibrated = calibrate(rightPupil, gaze, rH, errorMsg);

    switch(inputType) {
        case BINOCULAR:
            return binocularCalibrated;
        case BINOCULAR_MEAN_POR:
            return monoLeftCalibrated && monoRightCalibrated;
        case MONO_LEFT:
            return monoLeftCalibrated;
        case MONO_RIGHT:
            return monoRightCalibrated;
    }

    calibrationInputType = inputType;
    return true;
}

Point3f Homography::estimateGaze(const CollectionTuple &tuple, const InputType inputType)
{
    Point3f estimate(-1,-1,-1);

    vector<Point2d> ev;
    vector<Point2d> pupil;

    switch (inputType) {
        case BINOCULAR:
            if (!binocularCalibrated)
                return estimate;
            pupil.push_back( (tuple.lEye.pupil.center + tuple.rEye.pupil.center)/2 );
            perspectiveTransform(pupil, ev, mH);
            estimate = to3D(ev[0]);
            break;

        case BINOCULAR_MEAN_POR:
            if (!monoLeftCalibrated || !monoRightCalibrated)
                return estimate;
            pupil.push_back( tuple.lEye.pupil.center );
            perspectiveTransform(pupil, ev, lH);
            estimate = to3D(ev[0]);
            pupil.clear();
            pupil.push_back( tuple.rEye.pupil.center );
            perspectiveTransform(pupil, ev, rH);
            estimate = (estimate + to3D(ev[0])) / 2.0;
            break;

        case MONO_LEFT:
            if (!monoLeftCalibrated)
                return estimate;
            pupil.push_back( tuple.lEye.pupil.center );
            perspectiveTransform(pupil, ev, lH);
            estimate = to3D(ev[0]);
            break;

        case MONO_RIGHT:
            if (!monoRightCalibrated)
                return estimate;
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
