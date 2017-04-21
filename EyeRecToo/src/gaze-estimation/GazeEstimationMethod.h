#ifndef GAZEESTIMATIONMETHOD_H
#define GAZEESTIMATIONMETHOD_H

#include <vector>

#include <QString>
#include <QObject>

#include <opencv2/core.hpp>

#include "Synchronizer.h"

class CollectionTuple : public DataTuple
{
public:
    CollectionTuple(){
        tupleType = UNKNOWN;
    }

    cv::Mat fakeMat(cv::Mat in) {
        cv::Mat tmp;
        tmp.rows = in.rows;
        tmp.cols = in.cols;
        return tmp;
    }
    CollectionTuple(DataTuple dataTuple){
        timestamp = dataTuple.timestamp;
        lEye = dataTuple.lEye;
        rEye = dataTuple.rEye;
        field = dataTuple.field;
        // Keep everything but the input images for calibration.
        // This allows us to gather significantly more points without running out of memory
        lEye.input = fakeMat(lEye.input);
        rEye.input = fakeMat(rEye.input);
        field.input = fakeMat(field.input);
        tupleType = UNKNOWN;
        outlierDesc = OD_INLIER;
        autoEval = AE_NO;
    }

    enum TupleType {
        UNKNOWN = 0,
        CALIBRATION = 1,
        EVALUATION = 2,
    };
    TupleType tupleType;
    bool isCalibration() { return tupleType == CALIBRATION; }
    bool isEvaluation()  { return tupleType == EVALUATION; }

    enum OutlierDesc {
        OD_INLIER = 0,
        OD_MISSING_INPUT = 1,
        OD_PUPIL_RATIO = 2,
        OD_PUPIL_POSITION = 3,
    };
    OutlierDesc outlierDesc;
    bool isOutlier() { return outlierDesc != OD_INLIER; }

    enum AutoEval {
        AE_NO = 0,
        AE_YES = 1,
        AE_BIASED = 2,
    };
    AutoEval autoEval;
    bool isAutoEval() { return autoEval == AE_YES; }

    QString header(QString prefix = "") const {
        QString tmp;
        tmp.append(field.header("field.").append(prefix));
        tmp.append(lEye.header("left.").append(prefix));
        tmp.append(rEye.header("right.").append(prefix));
        tmp.append(gDataSeparator);
        return tmp;
    }
    QString toQString() const {
        QString tmp;
        tmp.append(field.toQString());
        tmp.append(lEye.toQString());
        tmp.append(rEye.toQString());
        tmp.append(gDataSeparator);
        return tmp;
    }

};

class GazeEstimationMethod
{
public:
    GazeEstimationMethod() {}
    ~GazeEstimationMethod() {}

    enum InputType{
        BINOCULAR   = 0,
        BINOCULAR_MEAN_POR   = 1,
        MONO_LEFT   = 2,
        MONO_RIGHT  = 3,
        UNKNOWN = 4
    };

    cv::Point3f gt;

    virtual bool calibrate(std::vector<CollectionTuple> &calibrationTuples, InputType inputType, QString &error) = 0;
    virtual cv::Point3f estimateGaze(const CollectionTuple &tuple, const InputType inputType) = 0;
    virtual std::string description() = 0;
};
Q_DECLARE_METATYPE(enum GazeEstimationMethod::InputType)

#endif // GAZEESTIMATIONMETHOD_H
