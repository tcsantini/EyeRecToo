#include "CameraCalibration.h"

using namespace cv;
using namespace std;

#define DBG_WINDOW_NAME "Camera Calibration Sample"

// TODO: handle user closing the calibration window during calibration

// TODO: move rectified map initialization and undistortion to a single interface;
// use this interface where appropriate (e.g., the image processors).

void CameraCalibration::toggleCalibration(bool status)
{
	Q_UNUSED(status);
	if (calibrationTogglePB->isChecked()) {
		QMetaObject::invokeMethod(this, "startCalibration", Qt::QueuedConnection);
	} else {
		// Set temporary GUI state during calculations since it might take some time
		calibrationTogglePB->setText("Calibrating...");
		calibrationTogglePB->setEnabled(false);
		collectPB->setEnabled(false);
		setLabelText(rmsQL, "Calculating", "orange");
		if (dbgCB->isChecked())
			destroyWindow(DBG_WINDOW_NAME);
		QMetaObject::invokeMethod(this, "finishCalibration", Qt::QueuedConnection);
	}
}

void CameraCalibration::startCalibration()
{
	calibrationTogglePB->setText("Finish");
	settingsGB->setEnabled(false);
	collectPB->setEnabled(true);
	undistortPB->setEnabled(false);
	imagePoints.clear();
	setCoverage();
	sampleCount = 0;
	sampleCountQL->setText(QString::number(sampleCount));
	covered = Rect();
	if (dbgCB->isChecked()) {
		namedWindow(DBG_WINDOW_NAME);
		this->activateWindow();
		this->setFocus();
	}
}

void CameraCalibration::finishCalibration()
{
	QFuture<void> future = QtConcurrent::run(this, &CameraCalibration::calibrate);
	watcher.setFuture(future);
}

void CameraCalibration::onCalibrated()
{
	this->layout()->setEnabled(true);
	updateCalibrationStatus( calibrationSuccessful );
	calibrationTogglePB->setText("Start");
	calibrationTogglePB->setEnabled(true);
	undistortPB->setEnabled(true);
	settingsGB->setEnabled(true);
	emit calibrationFinished( calibrationSuccessful );
}

void CameraCalibration::updateCalibrationStatus(bool success)
{
	if (success) {
		qInfo() << id << "calibrated. RMS Error =" << rms;
		undistortPB->setEnabled(true);
		setRms(rms);
		setCoverage(coverage);
		sampleCountQL->setText(QString::number(sampleCount));
	} else {
		qInfo() << id << "calibration failed.";
		undistortPB->setEnabled(false);
		setRms();
		setCoverage();
		sampleCountQL->setText(QString::number(sampleCount));
	}
}

void CameraCalibration::collectCalibrationSample()
{
	collectPB->setEnabled(false);
	emit requestSample();
}

void CameraCalibration::collectUndistortionSample()
{
	undistortPB->setEnabled(false);
	emit requestSample();
}

void CameraCalibration::calculateBoardCorners(vector<Point3f> &corners)
{
	Size boardSize(hPatternSizeSB->value(), vPatternSizeSB->value());
	float squareSize = 1e-3*squareSizeMMDSB->value();
	corners.clear();
	switch(patternCB->currentData().toInt()) {
		case CHESSBOARD:
		case CIRCLES_GRID:
			for( int i = 0; i < boardSize.height; ++i )
				for( int j = 0; j < boardSize.width; ++j )
					corners.push_back(Point3f(j*squareSize, i*squareSize, 0));
			break;
		case ASYMMETRIC_CIRCLES_GRID:
			for( int i = 0; i < boardSize.height; i++ )
				for( int j = 0; j < boardSize.width; j++ )
					corners.push_back(Point3f((2*j + i % 2)*squareSize, i*squareSize, 0));
			break;
		default:
			break;
	}
}

void CameraCalibration::newSample(const Mat &frame)
{
	if (calibrationTogglePB->isChecked())
		processSample(frame);
	else
		undistortSample(frame);
}

void CameraCalibration::undistortSample(const Mat &frame)
{
	if (undistortPB->isEnabled())
		return;
	Mat tmp;
	if (fishEyeCB->isChecked())
		remap(frame, tmp, map1, map2, CV_INTER_AREA);
	else
		remap(frame, tmp, map1, map2, CV_INTER_AREA);
	imshow("Undistorted Image", tmp);
	this->activateWindow();
	this->setFocus();
	undistortPB->setEnabled(true);
}

void CameraCalibration::processSample(const Mat &frame)
{
	if (collectPB->isEnabled())
		return;

	vector<Point2f> pointBuf;
	bool found;

	Mat gray = frame;
	if (frame.channels() == 3)
		cvtColor(frame, gray, CV_BGR2GRAY);

	imageSize = { gray.cols, gray.rows };

	Size patternSize( hPatternSizeSB->value(), vPatternSizeSB->value() );

	int chessBoardFlags = CALIB_CB_ADAPTIVE_THRESH | CALIB_CB_NORMALIZE_IMAGE;
	if( !fishEyeCB->isChecked())
		chessBoardFlags |= CALIB_CB_FAST_CHECK;

	switch( patternCB->currentData().toInt() )
	{
		case CHESSBOARD:
			found = findChessboardCorners( gray, patternSize, pointBuf, chessBoardFlags);
			if (found)
				cv::cornerSubPix( gray, pointBuf, Size(11,11), Size(-1,-1), TermCriteria( TermCriteria::EPS+TermCriteria::COUNT, 30, 0.1 ) );
			break;
		case CIRCLES_GRID:
			found = findCirclesGrid( gray, patternSize, pointBuf );
			break;
		case ASYMMETRIC_CIRCLES_GRID:
			found = findCirclesGrid( gray, patternSize, pointBuf, CALIB_CB_ASYMMETRIC_GRID );
			break;
		default:
			found = false;
			break;
	}

	if (found) {
		imagePoints.push_back(pointBuf);
		sampleCount = (int) imagePoints.size();
		if (sampleCount == 1)
			covered = boundingRect(pointBuf);
		else
			covered |= boundingRect(pointBuf);
		sampleCountQL->setText(QString::number(sampleCount));
		coverage = covered.area() / (double) imageSize.area();
		setCoverage( coverage );
	}

	if (dbgCB->isChecked()) {
		Mat tmp;
		if (frame.channels() == 1)
			cvtColor(frame, tmp, CV_GRAY2BGR);
		else
			tmp = frame.clone();

		if (found)
			drawChessboardCorners( tmp, patternSize, Mat(pointBuf), found);
		else
			putText(tmp, "Pattern not found", Point(0, 0.5*frame.rows), CV_FONT_HERSHEY_PLAIN, 2, Scalar(0,0,255));
		rectangle(tmp, covered, Scalar(0,255,0), 2);
		imshow(DBG_WINDOW_NAME, tmp);
	}

	collectPB->setEnabled(true);
}

void CameraCalibration::calibrate()
{
	qInfo() << "Calibrating, this might take some time; please wait.";
	cameraMatrix = Mat::eye(3, 3, CV_64F);
	if (fishEyeCB->isChecked())
		distCoeffs = Mat::zeros(4, 1, CV_64F);
	else
		distCoeffs = Mat::zeros(8, 1, CV_64F);

	calibrationSuccessful = false;
	rms = 0;

	if (imagePoints.size() < 5)
		return;

	vector<vector<Point3f> > objectPoints(1);
	calculateBoardCorners(objectPoints[0]);
	objectPoints.resize(imagePoints.size(),objectPoints[0]);

	try {
		Mat rv, tv;
		if (fishEyeCB->isChecked()) {
			int fisheyeFlags = 0;
			fisheyeFlags |= fisheye::CALIB_RECOMPUTE_EXTRINSIC;
			fisheyeFlags |= fisheye::CALIB_CHECK_COND;
			fisheyeFlags |= fisheye::CALIB_FIX_SKEW;
			rms = fisheye::calibrate(objectPoints, imagePoints, imageSize, cameraMatrix, distCoeffs, rv, tv, fisheyeFlags);
			fisheye::estimateNewCameraMatrixForUndistortRectify(cameraMatrix, distCoeffs, imageSize, Matx33d::eye(), newCameraMatrix, 1, imageSize);
			fisheye::initUndistortRectifyMap(cameraMatrix, distCoeffs, Matx33d::eye(),
								newCameraMatrix, imageSize, CV_16SC2,
								map1, map2);
		} else {
			rms = calibrateCamera(objectPoints, imagePoints, imageSize, cameraMatrix, distCoeffs, rv, tv);
			newCameraMatrix = getOptimalNewCameraMatrix(cameraMatrix, distCoeffs, imageSize, 1, imageSize);
			initUndistortRectifyMap( cameraMatrix, distCoeffs, Mat(),
								newCameraMatrix, imageSize, CV_16SC2,
								map1, map2
								);
		}
		calibrationSuccessful = true;
	} catch (cv::Exception &e) {
		qWarning() << "Calibration failed:" << e.what();
		calibrationSuccessful = false;
	}
}

void CameraCalibration::store(const QString &fileName)
{
	FileStorage fs( fileName.toStdString(), FileStorage::WRITE);
	fs << "cameraMatrix" << cameraMatrix;
	fs << "distCoeffs" << distCoeffs;
	fs << "imageSize" << imageSize;
	fs << "newCameraMatrix" << newCameraMatrix;
	fs << "sampleCount" << sampleCount;
	fs << "coverage" << coverage;
	fs << "rms" << rms;
	fs << "fisheye" << fishEyeCB->isChecked();
}

void CameraCalibration::load(const QString &fileName)
{
	QFileInfo info(fileName);
	if ( !info.exists() ) {
		calibrationSuccessful = false;
	} else {
		FileStorage fs( QString(fileName).toStdString(), FileStorage::READ);
		fs["cameraMatrix"] >> cameraMatrix;
		fs["distCoeffs"] >> distCoeffs;
		fs["imageSize"] >> imageSize;
		fs["newCameraMatrix"] >> newCameraMatrix;
		fs["sampleCount"] >> sampleCount;
		fs["coverage"] >> coverage;
		fs["rms"] >> rms;
		bool isFisheye;
		fs["fisheye"] >> isFisheye;
		calibrationSuccessful = true;
		if (isFisheye) {
			fisheye::initUndistortRectifyMap(cameraMatrix, distCoeffs, Matx33d::eye(),
							newCameraMatrix, imageSize, CV_16SC2,
							map1, map2);
		} else {
			initUndistortRectifyMap( cameraMatrix, distCoeffs, Mat(),
							newCameraMatrix, imageSize, CV_32FC1,
							map1, map2
							);
		}
	}
	updateCalibrationStatus(calibrationSuccessful);
}
