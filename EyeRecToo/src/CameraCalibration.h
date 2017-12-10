#ifndef CAMERACALIBRATION_H
#define CAMERACALIBRATION_H

#include <vector>

#include <QObject>
#include <QDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QFileInfo>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <QKeyEvent>

#include "opencv2/imgproc.hpp"
#include "opencv2/calib3d.hpp"
#include "opencv2/highgui.hpp"

#include "utils.h"

class CameraCalibration : public QDialog
{
	Q_OBJECT
public:
	CameraCalibration(QString id, QWidget *parent=0)
		: QDialog(parent),
		  calibrated(false),
		  sampleCount(0),
		  coverage(0),
		  id(id)
	{
		this->setWindowModality(Qt::ApplicationModal);
		this->setWindowTitle("Camera Calibration");

		QFormLayout *formLayout;
		QHBoxLayout *hl;

		hl = new QHBoxLayout();

		settingsGB = new QGroupBox("Settings");
		formLayout = new QFormLayout();
		hPatternSizeSB = new QSpinBox();
		hPatternSizeSB->setValue(4);
		hPatternSizeSB->setWhatsThis("Horizontal pattern size");
		hPatternSizeSB->setToolTip(hPatternSizeSB->whatsThis());
		formLayout->addRow( new QLabel("Horizontal:"), hPatternSizeSB );
		vPatternSizeSB = new QSpinBox();
		vPatternSizeSB->setValue(11);
		vPatternSizeSB->setWhatsThis("Vertical pattern size");
		vPatternSizeSB->setToolTip(vPatternSizeSB->whatsThis());
		formLayout->addRow( new QLabel("Vertical:"), vPatternSizeSB );
		squareSizeMMDSB = new QDoubleSpinBox();
		squareSizeMMDSB->setValue(20);
		squareSizeMMDSB->setSuffix(" mm");
		squareSizeMMDSB->setWhatsThis("Square size");
		squareSizeMMDSB->setToolTip(squareSizeMMDSB->whatsThis());
		formLayout->addRow( new QLabel("Square Size:"), squareSizeMMDSB );
		fishEyeCB = new QCheckBox();
		// TODO: should fisheye be the default?
		fishEyeCB->setChecked(false);
		fishEyeCB->setWhatsThis("Use fish eye camera model instead of pinhole.\nThis features is on alpha so use with care.");
		fishEyeCB->setToolTip(fishEyeCB->whatsThis());
		formLayout->addRow( new QLabel("Fish Eye"), fishEyeCB );
		dbgCB = new QCheckBox();
		dbgCB->setChecked(true);
		dbgCB->setWhatsThis("Display results from the pattern detection.");
		dbgCB->setToolTip(dbgCB->whatsThis());
		formLayout->addRow( new QLabel("Show Detection"), dbgCB );
		patternCB = new QComboBox();
		patternCB->addItem("Assymetric Circles", ASYMMETRIC_CIRCLES_GRID);
		patternCB->addItem("Chessboard", CHESSBOARD);
		patternCB->addItem("Circles", CIRCLES_GRID);
		patternCB->setWhatsThis("Calibration pattern");
		patternCB->setToolTip(patternCB->whatsThis());
		formLayout->addRow(patternCB);
		settingsGB->setLayout(formLayout);
		hl->addWidget(settingsGB);

		controlGB = new QGroupBox("Control");
		formLayout = new QFormLayout();
		calibrationTogglePB = new QPushButton();
		calibrationTogglePB->setText("Start");
		calibrationTogglePB->setCheckable(true);
		connect(calibrationTogglePB, SIGNAL(toggled(bool)),
				this, SLOT(toggleCalibration(bool)) );
		formLayout->addRow(calibrationTogglePB);
		collectPB = new QPushButton();
		collectPB->setText("Collect");
		connect(collectPB, SIGNAL(pressed()),
				this, SLOT(collectCalibrationSample()) );
		collectPB->setEnabled(false);
		formLayout->addRow(collectPB);
		undistortPB = new QPushButton();
		undistortPB->setText("Undistort");
		connect(undistortPB, SIGNAL(pressed()),
				this, SLOT(collectUndistortionSample()) );
		undistortPB->setEnabled(false);
		formLayout->addRow(undistortPB);
		sampleCountQL = new QLabel();
		sampleCountQL->setText(QString::number(0));
		formLayout->addRow( new QLabel("Samples:"), sampleCountQL);
		coveredQL = new QLabel();
		coveredQL->setText(QString::number(0) + " %");
		formLayout->addRow( new QLabel("Covered:"), coveredQL);
		rmsQL = new QLabel();
		setRms();
		formLayout->addRow( new QLabel("RMS Error:"), rmsQL);
		controlGB->setLayout(formLayout);
		hl->addWidget(controlGB);

		setLayout(hl);

		connect(&watcher, SIGNAL(finished()),
				this, SLOT(onCalibrated()) );

	}

	enum CALIBRATION_PATTERN {
		ASYMMETRIC_CIRCLES_GRID = 0,
		CIRCLES_GRID = 1,
		CHESSBOARD = 2,
	};


	void undistort(const cv::Mat &in, cv::Mat &out) {
		if ( !calibrated || in.cols != imageSize.width || in.rows != imageSize.height ) {
			out = in.clone();
			return;
		}

		if (fishEye)
			remap(in, out, map1, map2, CV_INTER_AREA);
		else
			remap(in, out, map1, map2, CV_INTER_AREA);
	}
	cv::Mat getCameraMatrix(cv::Size size) {
		if ( !calibrated || size != imageSize ) {
			return (cv::Mat_<double>(3,3) <<
						size.width, 0, 0.5*size.width,
						0, size.width, 0.5*size.height,
						0, 0 ,1 );

		}

		return cameraMatrix;
	}
	cv::Mat getDistortionCoefficients(cv::Size size) {
		if ( !calibrated || size != imageSize )
			return (cv::Mat_<double>(4,1) << 0, 0, 0, 0);

		// our coefficients are k1,k2,k3,k4, but estimatePoseSingleMarkers wants (k1,k2,p1,p2[,k3[,k4,k5,k6],[s1,s2,s3,s4]])
		if (fishEye)
			return (cv::Mat_<double>(4,1) << distCoeffs.at<double>(0,0), distCoeffs.at<double>(1,0), 0, 0);
		else
			return distCoeffs;
	}

signals:
	void requestSample();
	void calibrationFinished(bool success);

public slots:
	void showOptions(QPoint pos) {
		move(pos);
		show();
	}

	void toggleCalibration(bool status);
	void collectCalibrationSample();
	void collectUndistortionSample();
	void newSample(const cv::Mat &frame);
	void store(const QString &fileName);
	void load(const QString &fileName);

private:
	QGroupBox *settingsGB;
	QSpinBox *hPatternSizeSB;
	QSpinBox *vPatternSizeSB;
	QDoubleSpinBox *squareSizeMMDSB;
	QComboBox *patternCB;
	QCheckBox *fishEyeCB;
	QCheckBox *dbgCB;
	QGroupBox *controlGB;
	QPushButton *calibrationTogglePB;
	QPushButton *collectPB;
	QPushButton *undistortPB;
	QLabel *sampleCountQL;
	QLabel *coveredQL;
	QLabel *rmsQL;
	QString id;

	QFutureWatcher<void> watcher;

	std::vector<std::vector<cv::Point2f> > imagePoints;
	cv::Mat cameraMatrix;
	cv::Mat newCameraMatrix;
	cv::Mat distCoeffs;
	cv::Size imageSize;
	cv::Rect covered;
	int sampleCount;
	double coverage;
	double rms;
	bool fishEye;
	std::atomic<bool> calibrated;
	cv::Mat map1, map2;

	void processSample(const cv::Mat &frame);
	void undistortSample(const cv::Mat &frame);
	void calculateBoardCorners(std::vector<cv::Point3f> &corners);
	void calibrate();
	void updateCalibrationStatus(bool success);
	void internalInitUndistortRectifyMap();
	void setLabelText(QLabel* label, QString val, QString color) {
		label->setStyleSheet("QLabel { font : bold; color : " + color + " }");
		label->setText( val );
	}
	void setRms() { setLabelText(rmsQL, "N/A", "black"); }
	void setRms(double val) {
		if ( val <= 0.75) {
			setLabelText(rmsQL, QString::number(val), "green");
		} else if (val <= 1 ) {
			setLabelText(rmsQL, QString::number(val), "orange");
		} else {
			setLabelText(rmsQL, QString::number(val), "red");
			qInfo() << "RMS Error is above the expected value. It's recommended to recalibrate.";
		}
	}
	void setCoverage() { setLabelText(coveredQL, "N/A", "black"); }
	void setCoverage(double val) {
		QString pct = QString::number( (int) 100 * val) + " %";
		if ( val >= 0.8) {
			setLabelText(coveredQL, pct, "green");
		} else if (val >= 0.6) {
			setLabelText(coveredQL, pct, "orange");
		} else {
			setLabelText(coveredQL, pct, "red");
		}
	}

private slots:
	void startCalibration();
	void finishCalibration();
	void onCalibrated();
	void keyReleaseEvent(QKeyEvent *event) {
		// TODO: Document these
		if (event->isAutoRepeat())
			return;
		switch (event->key()) {
			case Qt::Key_S:
				calibrationTogglePB->click();
			break;
			case Qt::Key_C:
				collectPB->click();
			break;
			case Qt::Key_U:
				undistortPB->click();
			break;
			case Qt::Key_Escape:
				this->close();
			break;
		default:
			break;
		}
	}
};

#endif // CAMERACALIBRATION_H
