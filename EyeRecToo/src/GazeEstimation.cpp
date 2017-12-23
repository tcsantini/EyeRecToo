#include "GazeEstimation.h"

using namespace std;
using namespace cv;

GazeEstimation::GazeEstimation(QObject *parent)
    : QObject(parent),
      calibrated(false),
      isCalibrating(false),
	  gazeEstimationMethod(NULL),
	  lastOverlayIdx(0),
      settings(NULL)
{
    availableGazeEstimationMethods.push_back( new PolyFit(PolyFit::POLY_1_X_Y_XY_XX_YY_XYY_YXX_XXYY) );
    availableGazeEstimationMethods.push_back( new PolyFit(PolyFit::POLY_1_X_Y_XY) );
    availableGazeEstimationMethods.push_back( new PolyFit(PolyFit::POLY_1_X_Y_XY_XX_YY) );
    availableGazeEstimationMethods.push_back( new PolyFit(PolyFit::POLY_1_X_Y_XY_XX_YY_XXYY) );
    availableGazeEstimationMethods.push_back( new PolyFit(PolyFit::POLY_1_X_Y_XY_XX_YY_XYY_YXX) );
    availableGazeEstimationMethods.push_back( new PolyFit(PolyFit::POLY_1_X_Y_XY_XX_YY_XYY_YXX_XXX_YYY) );
    availableGazeEstimationMethods.push_back( new Homography() );
}
GazeEstimation::~GazeEstimation()
{
    for (int i=0; i<availableGazeEstimationMethods.size(); i++)
        delete availableGazeEstimationMethods[i];
    availableGazeEstimationMethods.clear();
}

void GazeEstimation::addTuple(CollectionTuple tuple)
{
    collectedTuples.push_back(tuple);
}
void GazeEstimation::addTuples(std::vector<CollectionTuple> tuples)
{
    collectedTuples.insert( collectedTuples.end(), tuples.begin(), tuples.end());
}
void GazeEstimation::reset(CollectionTuple::TupleType type)
{
    for (size_t i=collectedTuples.size(); i-->0;)
        if (collectedTuples[i].tupleType == type)
			collectedTuples.erase(collectedTuples.begin() + i);
}

bool GazeEstimation::isPupilOutlineValid(const EyeData &cur)
{
    if (cur.pupil.size.width <= 0 || cur.pupil.size.height <= 0)
        return false;
    return true;
}
bool GazeEstimation::isPupilRatioValid(const EyeData &prev, const EyeData &cur)
{
    // TODO: parametrize me
    int temporalThresholdMs = 500;
    double ratioThreshold = 0.75;

    // too far away in time, disconsider
    if (cur.timestamp - prev.timestamp > temporalThresholdMs)
        return true;

    std::pair<double, double> values = std::minmax(
                                               cur.pupil.size.area(),
                                               prev.pupil.size.area()
                                               );

    if (values.second == 0)
        return true;

    return (values.first / values.second < ratioThreshold) ? false : true;
}
void GazeEstimation::findPupilPositionOutliers(const Mat &pupil, Mat &mask)
{
    unsigned int outliers = 0;
    double sigmaThreshold = 2.7;
    while (true) {
        Scalar mu, s;
        meanStdDev(pupil, mu, s);
        for (int i=0; i<pupil.rows; i++) {
            if (mask.at<uchar>(i) == 0)
                continue;
            if (  pupil.at<Vec2f>(i)[0] < mu[0] - sigmaThreshold*s[0]
               || pupil.at<Vec2f>(i)[0] > mu[0] + sigmaThreshold*s[0]
               || pupil.at<Vec2f>(i)[1] < mu[1] - sigmaThreshold*s[1]
               || pupil.at<Vec2f>(i)[1] > mu[1] + sigmaThreshold*s[1]) {
                outliers++;
                mask.at<uchar>(i) = 0;
            }
        }
        if (outliers <= 0)
            break;
        outliers = 0;
    }
}
void GazeEstimation::detectOutliers()
{
    // reset outlier information
    for (unsigned int i=0; i<calibrationTuples.size(); i++)
        calibrationTuples[i]->outlierDesc = CollectionTuple::OD_INLIER;

    // Check for input type and pupil validity
    CollectionTuple *t;
    for (unsigned int i=0; i<calibrationTuples.size(); i++) {
        t = calibrationTuples[i];
        switch (cfg.inputType) {
            case GazeEstimationMethod::BINOCULAR:
            case GazeEstimationMethod::BINOCULAR_MEAN_POR:
                if ( !t->lEye.validPupil || ! t->rEye.validPupil )
                    t->outlierDesc = CollectionTuple::OD_MISSING_INPUT;
                break;
            case GazeEstimationMethod::MONO_LEFT:
                if ( !t->lEye.validPupil )
                    t->outlierDesc = CollectionTuple::OD_MISSING_INPUT;
                break;
            case GazeEstimationMethod::MONO_RIGHT:
                if ( !t->rEye.validPupil )
                    t->outlierDesc = CollectionTuple::OD_MISSING_INPUT;
                break;
        }
    }

    if (!cfg.removeOutliers)
        return;

    if (cfg.pupilRatioOutliers) {
        for (unsigned int i=1; i<calibrationTuples.size(); i++) {
            CollectionTuple *prev = calibrationTuples[i-1];
            CollectionTuple *cur = calibrationTuples[i];
            if (prev->isOutlier() || cur->isOutlier())
                continue;

            bool validLeft = isPupilRatioValid(prev->lEye, cur->lEye);
            bool validRight = isPupilRatioValid(prev->rEye, cur->rEye);

            switch (cfg.inputType) {
                case GazeEstimationMethod::BINOCULAR:
                case GazeEstimationMethod::BINOCULAR_MEAN_POR:
                    if (!validLeft || !validRight)
                        cur->outlierDesc = CollectionTuple::OD_PUPIL_RATIO;
                    break;
                case GazeEstimationMethod::MONO_LEFT:
                    if (!validLeft)
                        cur->outlierDesc = CollectionTuple::OD_PUPIL_RATIO;
                    break;
                case GazeEstimationMethod::MONO_RIGHT:
                    if (!validRight)
                        cur->outlierDesc = CollectionTuple::OD_PUPIL_RATIO;
                    break;
            }
        }
    }

    if (cfg.pupilPositionOutliers) {
        Mat lp(0, 0, CV_32FC2);
        Mat rp(0, 0, CV_32FC2);
        Mat mask(1, (int) calibrationTuples.size(), CV_8U);
        for (unsigned int i=0; i<calibrationTuples.size(); i++) {
            CollectionTuple *t = calibrationTuples[i];
            lp.push_back(t->lEye.pupil.center);
            rp.push_back(t->rEye.pupil.center);
            mask.at<uchar>(i) = t->isOutlier() ? 0 : 1;
        }
        if (cfg.inputType != GazeEstimationMethod::MONO_RIGHT)
                findPupilPositionOutliers(lp, mask);
        if (cfg.inputType != GazeEstimationMethod::MONO_LEFT)
                findPupilPositionOutliers(rp, mask);
        for (unsigned int i=0; i<calibrationTuples.size(); i++)
            if (mask.at<uchar>(i) == (uchar) 0)
                calibrationTuples[i]->outlierDesc = CollectionTuple::OD_PUPIL_POSITION;
    }

    if (cfg.pupilOutlineOutliers) {
        for (unsigned int i=0; i<calibrationTuples.size(); i++) {
            CollectionTuple *cur = calibrationTuples[i];
            if (cur->isOutlier())
                continue;

            bool validLeft = isPupilOutlineValid(cur->lEye);
            bool validRight = isPupilOutlineValid(cur->rEye);

            switch (cfg.inputType) {
                case GazeEstimationMethod::BINOCULAR:
                case GazeEstimationMethod::BINOCULAR_MEAN_POR:
                    if (!validLeft || !validRight)
                        cur->outlierDesc = CollectionTuple::OD_PUPIL_POSITION;
                    break;
                case GazeEstimationMethod::MONO_LEFT:
                    if (!validLeft)
                        cur->outlierDesc = CollectionTuple::OD_PUPIL_POSITION;
                    break;
                case GazeEstimationMethod::MONO_RIGHT:
                    if (!validRight)
                        cur->outlierDesc = CollectionTuple::OD_PUPIL_POSITION;
                    break;
            }
        }
    }

}

void GazeEstimation::calibrate()
{
    calibrated = false;
    autoVisualizationTimer.invalidate();
    errorVectors.clear();
    interpolationHull.clear();
    evaluationRegions.clear();

	QString error = "";
	if (!gazeEstimationMethod) {
		error = "No gaze estimation method available.";
		error = "No calibration tuples available.";
		qInfo() << error;
		emit calibrationFinished(false, error);
		return;
	}

    calibrationTuples.clear();

    for (unsigned int i=0; i<collectedTuples.size(); i++)
        if (collectedTuples[i].isCalibration())
            calibrationTuples.push_back(&collectedTuples[i]);
    if (calibrationTuples.size() <= 0) {
        error = "No calibration tuples available.";
        qInfo() << error;
        emit calibrationFinished(false, error);
        return;
    }

    detectOutliers();

    if (cfg.autoEvaluation)
        selectEvaluationTuples(cfg.granularity, cfg.horizontalStride, cfg.verticalStride, cfg.rangeFactor);
	else {
		for (unsigned int i=0; i<calibrationTuples.size(); i++) {
			calibrationTuples[i]->autoEval = CollectionTuple::AE_NO;
		}
	}

    vector<CollectionTuple> calibrationInliers;
    for (unsigned int i=0; i<calibrationTuples.size(); i++)
        if (!calibrationTuples[i]->isOutlier())
            calibrationInliers.push_back(*calibrationTuples[i]);

    updateInterpolationHull(calibrationInliers);

    qInfo() << "Using" << calibrationInliers.size() << "/" << calibrationTuples.size() << "calibration tuples.";
    if (calibrationInliers.size() <= 0) {
        error = "No calibration tuples remaining.";
        qInfo() << error;
        emit calibrationFinished(false, error);
        return;
    }

    calibrated = gazeEstimationMethod->calibrate(calibrationInliers, cfg.inputType, error);

    if (!calibrated) {
        qInfo() << error;
        emit calibrationFinished(false, error);
        return;
    }
	evaluate();

	if ( centralHullCoverage < cfg.minCentralAreaCoverage )
		error.append("Calibration didn't cover enough of the central area. ");
	if ( peripheralHullCoverage < cfg.minPeriphericAreaCoverage )
		error.append("Calibration didn't cover enough of the peripheric area. ");
	if ( 100*meanEvaluationError > cfg.maxReprojectionError )
		error.append("Reprojection error is too high. ");
	if (!error.isEmpty()) {
		qInfo() << error;
		emit calibrationFinished(false, error);
		calibrated = false;
		return;
	}

    autoVisualizationTimer.restart();
    emit calibrationFinished(true, "");
}

void GazeEstimation::estimate(DataTuple dataTuple)
{
    if (!gazeEstimationMethod)
        return;

    if (calibrated) {
        GazeEstimationMethod::InputType inputType = cfg.inputType;
		bool lValid = dataTuple.lEye.pupil.center.x > 0 && dataTuple.lEye.pupil.center.y > 0 && dataTuple.lEye.pupil.confidence > 0.66;
		bool rValid = dataTuple.rEye.pupil.center.x > 0 && dataTuple.rEye.pupil.center.y > 0 && dataTuple.rEye.pupil.confidence > 0.66;

        switch(inputType) {
            case GazeEstimationMethod::BINOCULAR:
            case GazeEstimationMethod::BINOCULAR_MEAN_POR:
                if (!lValid && !rValid) // None valid
                    break;
                if (lValid && !rValid) // Use only left
                    inputType = GazeEstimationMethod::MONO_LEFT;
                if (rValid && !lValid) // Use only right
                    inputType = GazeEstimationMethod::MONO_RIGHT;
                dataTuple.field.gazeEstimate = gazeEstimationMethod->estimateGaze(dataTuple, inputType);
                dataTuple.field.validGazeEstimate = true;
                break;
            case GazeEstimationMethod::MONO_LEFT:
                if (!lValid)
                    break;
                dataTuple.field.gazeEstimate = gazeEstimationMethod->estimateGaze(dataTuple, inputType);
                dataTuple.field.validGazeEstimate = true;
                break;
            case GazeEstimationMethod::MONO_RIGHT:
                if (!rValid)
                    break;
                dataTuple.field.gazeEstimate = gazeEstimationMethod->estimateGaze(dataTuple, inputType);
                dataTuple.field.validGazeEstimate = true;
                break;
        }

    }

    drawGazeEstimationInfo(dataTuple);

    emit gazeEstimationDone(dataTuple);
}

void GazeEstimation::printAccuracyInfo(const cv::Mat &errors, const QString &which, const double &diagonal, float &mu, float &sigma)
{
    Scalar m, std;
    meanStdDev(errors, m, std);
    qInfo() << which.toLatin1().data() <<  "Error ( N =" << errors.rows << "):";
    //qInfo() << QString("m = %1 ( std = %2 ) pixels."
    //    ).arg(m[0], 6, 'f', 2
	//    ).arg(std[0], 6, 'f', 2).toLatin1().data();
	mu = m[0] / diagonal;
	sigma = std[0] / diagonal;
	qInfo() << QString("m = %1 ( std = %2 ) % of the image diagonal."
		).arg(100 * mu, 6, 'f', 2
		).arg(100 * sigma, 6, 'f', 2).toLatin1().data();
}

void GazeEstimation::evaluate()
{
    if (!calibrated)
        return;

    evaluationTuples.clear();
    for (unsigned int i=0; i<collectedTuples.size(); i++)
        if (collectedTuples[i].isEvaluation() || (cfg.autoEvaluation && collectedTuples[i].isAutoEval()) )
			evaluationTuples.push_back(&collectedTuples[i]);

	centralHullCoverage = 0;
	peripheralHullCoverage = 0;
	if (!interpolationHull.empty()) {
		float delta = 0.25;
		Size size(calibrationTuples[0]->field.width, calibrationTuples[0]->field.height);
		Mat centralRegion = Mat::zeros(size, CV_8U);
		rectangle(centralRegion, Point(delta*size.width, delta*size.height), Point((1-delta)*size.width, (1-delta)*size.height), Scalar(255), -1);
		Mat peripheralRegion = 255 - centralRegion;

		Mat hullRegion = Mat::zeros(size, CV_8U);
		vector< vector<Point> > contours;
		contours.push_back(interpolationHull);
		drawContours(hullRegion, contours, 0, Scalar(255), -1);

		vector<Point> nonZero;
		findNonZero(centralRegion, nonZero);
		float centralPixels = nonZero.size();
		findNonZero(peripheralRegion, nonZero);
		float peripheralPixels = nonZero.size();

		Mat intersection;
		bitwise_and( hullRegion, centralRegion, intersection);
		findNonZero(intersection, nonZero);
		centralHullCoverage = nonZero.size() / centralPixels;

		bitwise_and( hullRegion, peripheralRegion, intersection);
		findNonZero(intersection, nonZero);
		peripheralHullCoverage = nonZero.size() / peripheralPixels;

		qInfo() << QString("Central Interpolation Coverage: %1 %"
					   ).arg(100 * centralHullCoverage , 0, 'f', 2).toLatin1().data();
		qInfo() << QString("Peripheral Interpolation Coverage: %1 %"
					   ).arg(100 * peripheralHullCoverage , 0, 'f', 2).toLatin1().data();
	}

	meanEvaluationError = 0;
	stdEvaluationError = 0;
	evaluationRegionCoverage = 1;

    if (evaluationTuples.size() <= 0 || calibrationTuples.size() <= 0)
		return;

	// evaluate for known points
    errorVectors.clear();
    Mat errors;
    for (unsigned int i=0; i<evaluationTuples.size(); i++) {
        Point3f gt = evaluationTuples[i]->field.collectionMarker.center;
        Point3f gaze = gazeEstimationMethod->estimateGaze(*evaluationTuples[i], cfg.inputType);
        errorVectors.push_back(ErrorVector(gt, gaze));
        errors.push_back( errorVectors.back().magnitude() );
    }

    double imDiagonal = sqrt( pow(evaluationTuples[0]->field.width,2) + pow(evaluationTuples[0]->field.height, 2) );
	printAccuracyInfo(errors, "Gaze Evaluation", imDiagonal, meanEvaluationError, stdEvaluationError);

    if (cfg.autoEvaluation) {
        double evaluationRegionsCount = pow(1 + 2*cfg.granularity, 2);
        double evaluationRegionsSelected = 0;
        for (unsigned int i=0; i<collectedTuples.size(); i++)
            if ( collectedTuples[i].isAutoEval() )
                evaluationRegionsSelected++;
		evaluationRegionCoverage = evaluationRegionsSelected / evaluationRegionsCount;
		qInfo() << QString("Auto Evaluation Region Coverage: %1 %"
						   ).arg(100 * evaluationRegionCoverage, 0, 'f', 2).toLatin1().data();
	}


}

/*
 * File loading / saving
 */
template<typename T> static void addData(map<QString, uint> &idxMap, QStringList &tokens, QString key, T &data)
{
    if (idxMap.count(key) == 0)
        return;

    data = QVariant(tokens[idxMap[key]]).value<T>();
}
void GazeEstimation::loadTuplesFromFile(CollectionTuple::TupleType tupleType, QString fileName)
{
    QFile inputFile(fileName);
    if (!inputFile.open(QIODevice::ReadOnly))
        return;
    QTextStream in(&inputFile);
    QStringList header;
    map<QString, uint> idxMap;
    vector<CollectionTuple> tuples;
    while (!in.atEnd())
    {
        QString line = in.readLine();
        QStringList tokens = line.split(gDataSeparator);
        CollectionTuple tuple;

        if (header.isEmpty()) {
            header = tokens;
            for( int i=0; i<tokens.size(); i++ )
                idxMap[tokens[i]] = i;
        } else {
            addData(idxMap, tokens, "timestamp", tuple.timestamp);
            // Field
            addData(idxMap, tokens, "field.timestamp", tuple.field.timestamp);
            addData(idxMap, tokens, "field.gaze.x", tuple.field.gazeEstimate.x);
            addData(idxMap, tokens, "field.gaze.y", tuple.field.gazeEstimate.y);
            addData(idxMap, tokens, "field.gaze.z", tuple.field.gazeEstimate.z);
            addData(idxMap, tokens, "field.gaze.valid", tuple.field.validGazeEstimate);
            addData(idxMap, tokens, "field.collectionMarker.id", tuple.field.collectionMarker.id);
            addData(idxMap, tokens, "field.collectionMarker.x", tuple.field.collectionMarker.center.x);
            addData(idxMap, tokens, "field.collectionMarker.y", tuple.field.collectionMarker.center.y);
            addData(idxMap, tokens, "field.collectionMarker.z", tuple.field.collectionMarker.center.z);
            addData(idxMap, tokens, "field.undistorted", tuple.field.undistorted);
            addData(idxMap, tokens, "field.width", tuple.field.width);
            addData(idxMap, tokens, "field.height", tuple.field.height);
            // TODO: read other markers

            // Left Eye
            addData(idxMap, tokens, "left.timestamp", tuple.lEye.timestamp);
            addData(idxMap, tokens, "left.pupil.x", tuple.lEye.pupil.center.x);
            addData(idxMap, tokens, "left.pupil.y", tuple.lEye.pupil.center.y);
            addData(idxMap, tokens, "left.pupil.width", tuple.lEye.pupil.size.width);
            addData(idxMap, tokens, "left.pupil.height", tuple.lEye.pupil.size.height);
            addData(idxMap, tokens, "left.pupil.angle", tuple.lEye.pupil.angle);
            addData(idxMap, tokens, "left.pupil.valid", tuple.lEye.validPupil);

            // Right Eye
            addData(idxMap, tokens, "right.timestamp", tuple.rEye.timestamp);
            addData(idxMap, tokens, "right.pupil.x", tuple.rEye.pupil.center.x);
            addData(idxMap, tokens, "right.pupil.y", tuple.rEye.pupil.center.y);
            addData(idxMap, tokens, "right.pupil.width", tuple.rEye.pupil.size.width);
            addData(idxMap, tokens, "right.pupil.height", tuple.rEye.pupil.size.height);
            addData(idxMap, tokens, "right.pupil.angle", tuple.rEye.pupil.angle);
            addData(idxMap, tokens, "right.pupil.valid", tuple.rEye.validPupil);

            // Add
            tuple.tupleType = tupleType;
            tuples.push_back(tuple);
        }
    }

    addTuples(tuples);
    inputFile.close();

    calibrate();
}

void GazeEstimation::saveTuplesToFile(CollectionTuple::TupleType tupleType, QString fileName)
{
	vector<CollectionTuple*> pertinentTuples;
	for (unsigned int i=0; i<collectedTuples.size(); i++) {
		//if ( tupleType == CollectionTuple::CALIBRATION)
		//	if (collectedTuples[i].isEvaluation())
		//		continue;

		if ( tupleType == CollectionTuple::EVALUATION) {
			if (collectedTuples[i].isCalibration())
				continue;
		}

		pertinentTuples.push_back(&collectedTuples[i]);
	}

	saveTuplesToFile(pertinentTuples, fileName, QIODevice::WriteOnly);
}

void GazeEstimation::saveTuplesToFile(const std::vector<CollectionTuple *> &tuples, QString fileName, QFlags<QIODevice::OpenModeFlag> flags)
{
    if (fileName.isEmpty() || tuples.size() == 0)
        return;

    QString buffer;
    buffer.append(tuples[0]->header() + gDataNewline);
    for (unsigned int i=0; i<tuples.size(); i++)
        buffer.append(tuples[i]->toQString() + gDataNewline);

    QFile f(fileName);
    f.open(flags);
    QTextStream s(&f);
    if (s.status() == QTextStream::Ok)
        s << buffer.toStdString().c_str();
    f.close();
}

void GazeEstimation::updateConfig()
{
    cfg.load(settings);
    for (int i=0; i<availableGazeEstimationMethods.size(); i++) {
        QString name = availableGazeEstimationMethods[i]->description().c_str();
        if (cfg.gazeEstimationMethod == name)
            gazeEstimationMethod = availableGazeEstimationMethods[i];
    }
    calibrate();
}

void GazeEstimation::selectEvaluationTuples(const int g,  const double dx, const double dy, const double rf)
{
    vector<CollectionTuple*> evaluationCandidates;
    vector<Point> points;
    CollectionTuple* t;

    evaluationRegions.clear();

    /*
     * collect non-outlier candidates
     */
    for (unsigned int i=0; i<calibrationTuples.size(); i++) {
        t = calibrationTuples[i];
        t->autoEval = CollectionTuple::AE_NO;
        if (t->isOutlier())
            continue;
        evaluationCandidates.push_back(t);
        points.push_back( to2D(t->field.collectionMarker.center) );
    }

    if (evaluationCandidates.size() <= 0)
        return;

    /*
     *  exclude points in the hull so we don't lose interpolation range
     */
    convexHull(points, interpolationHull);

//#define DBG_AUTOEVAL
#ifdef DBG_AUTOEVAL
    const Point *ep[1] = { &interpolationHull[0] };
    int epn = (int) interpolationHull.size();
    Mat dbg = Mat::zeros( evaluationCandidates[0]->field.height, evaluationCandidates[0]->field.width, CV_8UC3);
    fillPoly( dbg, ep, &epn, 1, Scalar(255,255,255));
    for (size_t i=evaluationCandidates.size(); i-->0;) {
        Point gaze =  to2D( evaluationCandidates[i]->field.collectionMarker.center );
        circle(dbg, gaze, 5, Scalar(0,255,255), -1);
    }
#endif

    for (size_t i=evaluationCandidates.size(); i-->0;) {
        Point gaze =  to2D( evaluationCandidates[i]->field.collectionMarker.center );
        for (unsigned int j=0; j<interpolationHull.size(); j++) {
            if ( ED(gaze,interpolationHull[j]) <= 1.5 ) {
#ifdef DBG_AUTOEVAL
                circle(dbg, gaze, 5, Scalar(255,0,0), -1);
#endif
                evaluationCandidates.erase(evaluationCandidates.begin()+i);
                break;
            }
        }
    }

    /*
     * Select points that minimize distance to evaluation region center for evaluation
     */
    Point center(t->field.width/2, t->field.height/2);
    int dxPx = dx*t->field.width;
    int dyPx = dy*t->field.height;
    int w = rf*dxPx;
    int h = rf*dyPx;
    vector<Point> evaluationPoints;
    for (int x = -g; x <= g; x++) {
        for (int y = -g; y <= g; y++) {
            EvaluationRegion er( center.x + x*dxPx, center.y + y*dyPx, w, h );
            for (int i=0; i<evaluationCandidates.size(); i++)
                er.eval(evaluationCandidates[i]);
            if (er.selected) {
                er.selected->autoEval = CollectionTuple::AE_YES;
                evaluationPoints.push_back(to2D(er.selected->field.collectionMarker.center));
            }
            evaluationRegions.push_back(er);
#ifdef DBG_AUTOEVAL
            er.draw(dbg);
#endif
        }
    }

    /*
     * Mark calibration points that could bias the auto evaluation
     */
    for (unsigned int i=0; i<evaluationPoints.size(); i++) {
        Point ep = evaluationPoints[i];
        for (unsigned int j=0; j<evaluationCandidates.size(); j++) {
            t = evaluationCandidates[j];
            if (t->autoEval == CollectionTuple::AE_NO) {
                Point cp = to2D(t->field.collectionMarker.center);
                if ( ED(ep, cp) < 1.5 ) {
                    t->autoEval = CollectionTuple::AE_BIASED;
#ifdef DBG_AUTOEVAL
                    circle( dbg, cp, 2, Scalar(255,255,0), -1);
#endif
                }
            }
        }
    }

#ifdef DBG_AUTOEVAL
    imshow("dbg", dbg);
#endif
}

void GazeEstimation::drawGazeEstimationInfo(DataTuple &dataTuple)
{
    dataTuple.showGazeEstimationVisualization = false;

    // TODO: improve this? maybe draw on an overlay canvas that is only updated
    // when something changes. Then we overlay the canvas on a copy of the image
    if (dataTuple.field.input.empty() || !cfg.visualize)
        return;

    bool shouldDisplay = false;
    if (isCalibrating)
        shouldDisplay = true;
    else {
        if (autoVisualizationTimer.isValid()) {
            if (autoVisualizationTimer.elapsed() < cfg.visualizationTimeS*1e3)
                shouldDisplay = true;
            else
                autoVisualizationTimer.invalidate();
        }
    }
	if (!shouldDisplay) {
		lastOverlayIdx = 0;
		return;
	}

	dataTuple.showGazeEstimationVisualization = true;
	dataTuple.gazeEstimationVisualization = vis;

	// avoid drawing every single frame
	//static Timestamp lastGazeEstimationVisualizationTimestamp = 0;
	//Timestamp current = gTimer.elapsed();
	//bool shouldDraw = current - lastGazeEstimationVisualizationTimestamp > 40;
	//if (!shouldDraw)
	//	return;
	//lastGazeEstimationVisualizationTimestamp = current;

	vis  = dataTuple.field.input.clone();
    int r = max<int>( 1, 0.003125*max<int>(vis.rows, vis.cols) );

	if (isCalibrating) {

		if (lastOverlayIdx > collectedTuples.size()) // sample removed, restart
			lastOverlayIdx = 0;

		if (lastOverlayIdx == 0)
			overlay = Mat::zeros( vis.rows, vis.cols, CV_8UC3);

		for ( ; lastOverlayIdx < collectedTuples.size(); lastOverlayIdx++) {
			circle(overlay, to2D(collectedTuples[lastOverlayIdx].field.collectionMarker.center), r, CV_ALMOST_BLACK, -1);
			if (collectedTuples[lastOverlayIdx].isCalibration())
				circle(overlay, to2D(collectedTuples[lastOverlayIdx].field.collectionMarker.center), r+1, CV_GREEN, 0.5*r);
			else
				circle(overlay, to2D(collectedTuples[lastOverlayIdx].field.collectionMarker.center), r+1, CV_CYAN, 0.5*r);
		}

	} else {
		// Calibration finished; overlay results and restart
		overlay = Mat::zeros( vis.rows, vis.cols, CV_8UC3);
		lastOverlayIdx = 0;

		if (!interpolationHull.empty()) {
            vector< vector<Point> > contours;
            contours.push_back(interpolationHull);
			drawContours(overlay, contours, 0, CV_GREEN, r);
        }

        for (size_t i=0; i<errorVectors.size(); i++)
			errorVectors[i].draw(overlay, 2, CV_RED);

        if (cfg.autoEvaluation)
            for (size_t i=0; i<evaluationRegions.size(); i++)
				evaluationRegions[i].draw(overlay, r);

		for (size_t i=0; i<collectedTuples.size(); i++) {
			if (!collectedTuples[i].isCalibration()) {
				circle(overlay, to2D(collectedTuples[i].field.collectionMarker.center), r, CV_BLACK, -1);
				circle(overlay, to2D(collectedTuples[i].field.collectionMarker.center), r+1, CV_CYAN, 0.5*r);
			}
		}

	}

	// Overlay on visualization image; notice we use CV_ALMOST_BLACK instead of
	// CV_BLACK so we don't need to create an additional mask :-)
	overlay.copyTo(vis, overlay);
}

void GazeEstimation::setCalibrating(bool v)
{
    isCalibrating = v;
}

void GazeEstimation::updateInterpolationHull(const std::vector<CollectionTuple> &tuples)
{
    if (tuples.size() <= 0)
        return;

    vector<Point> points;
    for (unsigned int i=0; i<tuples.size(); i++)
        points.push_back( to2D(tuples[i].field.collectionMarker.center) );

    convexHull(points, interpolationHull);
}
