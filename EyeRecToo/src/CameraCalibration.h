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

#include "opencv2/imgproc.hpp"
#include "opencv2/calib3d.hpp"
#include "opencv2/highgui.hpp"

#include "utils.h"

class CameraCalibration : public QDialog
{
	Q_OBJECT
public:
	CameraCalibration(QWidget *parent=0)
		: QDialog(parent),
		  calibrationSuccessful(false)
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
		fishEyeCB->setChecked(false);
		fishEyeCB->setWhatsThis("Use fish eye camera model instead of pinhole. Untested.");
		fishEyeCB->setToolTip(fishEyeCB->whatsThis());
		// TODO: test fish eye. Until then, disable it
		fishEyeCB->setEnabled(false);
		formLayout->addRow( new QLabel("Fish Eye"), fishEyeCB );
		dbgCB = new QCheckBox();
		dbgCB->setChecked(false);
		dbgCB->setWhatsThis("Display results from the pattern detection.");
		dbgCB->setToolTip(dbgCB->whatsThis());
		formLayout->addRow( new QLabel("Show Debug"), dbgCB );
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
		rmsQL = new QLabel();
		rmsQL->setStyleSheet("QLabel { font : bold; color : black }");
		rmsQL->setText("N/A");
		formLayout->addRow( new QLabel("RMS Error:"), rmsQL);
		controlGB->setLayout(formLayout);
		hl->addWidget(controlGB);

		setLayout(hl);
	}

	enum CALIBRATION_PATTERN {
		ASYMMETRIC_CIRCLES_GRID = 0,
		CIRCLES_GRID = 1,
		CHESSBOARD = 2,
	};

	cv::Mat cameraMatrix;
	cv::Mat newCameraMatrix;
	cv::Mat distCoeffs;
	bool calibrationSuccessful;

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
	QLabel *rmsQL;

	std::vector<std::vector<cv::Point2f> > imagePoints;
	cv::Size imageSize;
	void processSample(const cv::Mat &frame);
	void undistortSample(const cv::Mat &frame);
	void calculateBoardCorners(std::vector<cv::Point3f> &corners);
	void calibrate();
	void updateCalibrationStatus(bool success);

	double rms;

private slots:
	void startCalibration();
	void finishCalibration();
};

#endif // CAMERACALIBRATION_H
