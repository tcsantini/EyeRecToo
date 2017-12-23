#ifndef GAZEESTIMATION_H
#define GAZEESTIMATION_H

#include <vector>
#include <deque>
#include <map>

#include <QObject>

#include "gaze-estimation/GazeEstimationMethod.h"
#include "gaze-estimation/PolyFit.h"
#include "gaze-estimation/Homography.h"

#include "Reference.h"

/*******************************************************************************
 * Helper Classes
 ******************************************************************************/
class EvaluationRegion {
public:
    EvaluationRegion(int x, int y, int w, int h)
    : x(x), y(y), w(w), h(h),
      rw2(pow(w/2.0,2)),
      rh2(pow(h/2.0,2)),
      mn(DBL_MAX),
      selected(NULL) {}
    int x, y, w, h, rw2, rh2;
    double mn;
    CollectionTuple *selected;
    void eval(CollectionTuple* t) {
    cv::Point p = to2D(t->field.collectionMarker.center);
    double d = (pow(p.x - x, 2) / rw2  + pow(p.y - y, 2) / rh2);
    if ( d <= 1 && d < mn) {
        mn = d;
        selected = t;
    }
    }
    void draw(cv::Mat &in, int r = 3) {
        cv::RotatedRect el(cv::Point(x,y), cv::Size(w,h), 0);
        if (selected){
            cv::ellipse(in, el, CV_GREEN, 0.5*r);
			cv::circle( in, to2D(selected->field.collectionMarker.center), r, CV_ALMOST_BLACK, -1);
            cv::circle( in, to2D(selected->field.collectionMarker.center), r+1, CV_GREEN, r);
        } else
            cv::ellipse(in, el, CV_RED, 0.5*r);
    }
};

class ErrorVector {
public:
    ErrorVector(cv::Point3f gt, cv::Point3f gaze) :
        gt(gt), gaze(gaze) {}

    cv::Point3f gt;
    cv::Point3f gaze;
    void draw(cv::Mat &in, int w = 2, cv::Scalar color = CV_RED) {
        cv::line( in, cv::Point(gt.x, gt.y), cv::Point(gaze.x, gaze.y), color, w);
    }
    double magnitude() { return cv::norm(gt - gaze); }
};

/*******************************************************************************
 * Gaze Estimation
 ******************************************************************************/
class GazeEstimationConfig
{
public:
    GazeEstimationConfig() :
        samplingTimeMs(500),
        useMedian(false),
        removeOutliers(true),
        pupilRatioOutliers(true),
        pupilPositionOutliers(true),
        pupilOutlineOutliers(true),
        autoEvaluation(true),
        granularity(2),
        horizontalStride(0.15),
        verticalStride(0.15),
        rangeFactor(0.95),
        inputType(GazeEstimationMethod::BINOCULAR_MEAN_POR),
        gazeEstimationMethod("POLY_X_Y_XY_XX_YY_XYY_YXX_XXYY"),
        visualize(true),
		visualizationTimeS(5),
		minCentralAreaCoverage(0.8f),
		minPeriphericAreaCoverage(0.1f),
		maxReprojectionError(4.f)
	{}

    /*
     * Gaze Estimation Widget
     */
    int samplingTimeMs;
    bool useMedian;

    /*
     * Gaze Estimation
     */
    // outlier removal
    bool removeOutliers;
    bool pupilRatioOutliers;
    bool pupilPositionOutliers;
    bool pupilOutlineOutliers;

    // Method
    GazeEstimationMethod::InputType inputType;
    QString gazeEstimationMethod;

    // Visualization
    bool visualize;
    int visualizationTimeS;

    // CalibMe
    bool autoEvaluation;
    int granularity;
    double horizontalStride;
    double verticalStride;
    double rangeFactor;

	// Quality check
	float minCentralAreaCoverage;
	float minPeriphericAreaCoverage;
	float maxReprojectionError;

    void save(QSettings *settings)
    {
        settings->sync();
        settings->setValue("samplingTimeMs", samplingTimeMs);
        settings->setValue("useMedian", useMedian);
        settings->setValue("inputType", inputType);
        settings->setValue("gazeEstimationMethod", gazeEstimationMethod);
        settings->setValue("visualize", visualize);
        settings->setValue("visualizationTimeS", visualizationTimeS);

        // CalibMe
        settings->setValue("CalibMe/removeOutliers", removeOutliers);
        settings->setValue("CalibMe/pupilRatioOutliers", pupilRatioOutliers);
        settings->setValue("CalibMe/pupilPositionOutliers", pupilPositionOutliers);
        settings->setValue("CalibMe/pupilOutlineOutliers", pupilOutlineOutliers);
        settings->setValue("CalibMe/autoEvaluation", autoEvaluation);
        settings->setValue("CalibMe/granularity", granularity);
        settings->setValue("CalibMe/horizontalStride", horizontalStride);
        settings->setValue("CalibMe/verticalStride", verticalStride);
		settings->setValue("CalibMe/rangeFactor", rangeFactor);

		// Quality check
		settings->setValue("quality/minCentralAreaCoverage", minCentralAreaCoverage);
		settings->setValue("quality/minPeriphericAreaCoverage", minPeriphericAreaCoverage);
		settings->setValue("quality/maxReprojectionError", maxReprojectionError);
	}

    void load(QSettings *settings)
    {
        settings->sync();
        set(settings, "samplingTimeMs", samplingTimeMs);
        set(settings, "useMedian", useMedian);
        set(settings, "inputType", inputType);
        set(settings, "gazeEstimationMethod", gazeEstimationMethod );
        set(settings, "visualize", visualize);
        set(settings, "visualizationTimeS", visualizationTimeS);

        //CalibMe
        set(settings, "CalibMe/removeOutliers", removeOutliers       );
        set(settings, "CalibMe/pupilRatioOutliers", pupilRatioOutliers   );
        set(settings, "CalibMe/pupilPositionOutliers", pupilPositionOutliers);
        set(settings, "CalibMe/pupilOutlineOutliers", pupilOutlineOutliers );
        set(settings, "CalibMe/autoEvaluation", autoEvaluation       );
        set(settings, "CalibMe/granularity", granularity          );
        set(settings, "CalibMe/horizontalStride", horizontalStride     );
        set(settings, "CalibMe/verticalStride", verticalStride       );
		set(settings, "CalibMe/rangeFactor", rangeFactor          );

		set(settings, "quality/minCentralAreaCoverage", minCentralAreaCoverage );
		set(settings, "quality/minPeriphericAreaCoverage", minPeriphericAreaCoverage );
		set(settings, "quality/maxReprojectionError", maxReprojectionError );
	}
};

class GazeEstimation : public QObject
{
    Q_OBJECT

public:
    explicit GazeEstimation(QObject *parent = 0);
    ~GazeEstimation();
    QSettings *settings;
    std::vector<GazeEstimationMethod*> availableGazeEstimationMethods;

signals:
    void gazeEstimationDone(DataTuple dataTuple);
    void calibrationFinished(bool status, QString msg);

public slots:
    void loadTuplesFromFile(CollectionTuple::TupleType type, QString fileName);
    void saveTuplesToFile(CollectionTuple::TupleType type, QString fileName);
    void saveTuplesToFile(const std::vector<CollectionTuple*> &tuples, QString fileName, QFlags<QIODevice::OpenModeFlag> flags);
    void addTuple(CollectionTuple tuple);
    void addTuples(std::vector<CollectionTuple> tuples);
	void reset(CollectionTuple::TupleType type);
	void saveCalibration() { saveTuplesToFile( CollectionTuple::TupleType::CALIBRATION, QString("%1-calibration.tup").arg( gTimer.elapsed() ) ); }

    void setCalibrating(bool v);

    void calibrate();
    void estimate(DataTuple dataTuple);
    void evaluate();
	void printAccuracyInfo(const cv::Mat &errors, const QString &which, const double &diagonal, float &mu, float &sigma);

    void updateConfig();

private:
    bool calibrated;
    bool isCalibrating;
    GazeEstimationConfig cfg;
    std::vector<CollectionTuple> collectedTuples;
    std::vector<CollectionTuple*> calibrationTuples;
    std::vector<CollectionTuple*> evaluationTuples;


    GazeEstimationMethod *gazeEstimationMethod;
    QMutex cfgMutex;

	float centralHullCoverage;
	float peripheralHullCoverage;
	float evaluationRegionCoverage;
	float meanEvaluationError;
	float stdEvaluationError;

	std::vector<cv::Point> interpolationHull;
    std::vector<EvaluationRegion> evaluationRegions;
    std::vector<ErrorVector> errorVectors;
    QElapsedTimer autoVisualizationTimer;
    cv::Mat vis;
	cv::Mat overlay;
	int lastOverlayIdx;


private slots:
    void detectOutliers();
    bool isPupilRatioValid(const EyeData &prev, const EyeData &cur);
    void findPupilPositionOutliers(const cv::Mat &pupil, cv::Mat &mask);
    bool isPupilOutlineValid(const EyeData &cur);
    void selectEvaluationTuples(const int g, const double dx, const double dy, const double rf);

    void updateInterpolationHull(const std::vector<CollectionTuple> &tuples);
    void drawGazeEstimationInfo(DataTuple &dataTuple);
};

#endif // GAZEESTIMATION_H
