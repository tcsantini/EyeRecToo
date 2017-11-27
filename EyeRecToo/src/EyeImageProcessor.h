#ifndef EYEIMAGEPROCESSOR_H
#define EYEIMAGEPROCESSOR_H

#include <QObject>
#include <QApplication>
#include <QDialog>
#include <QGridLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QSpinBox>
#include <QSettings>
#include <QGroupBox>
#include <QCheckBox>
#include <QFormLayout>

#include <opencv/cv.h>

#include "InputWidget.h"

#include "pupil-detection/PuRe.h"
#include "pupil-detection/ElSe.h"
#include "pupil-detection/ExCuSe.h"
#ifdef STARBURST
#include "pupil-detection/Starburst.h"
#endif
#ifdef SWIRSKI
#include "pupil-detection/Swirski.h"
#endif
#include "pupil-detection/PupilDetectionMethod.h"

#include "pupil-tracking/PuReTy.h"
#include "pupil-tracking/PupilTrackingMethod.h"

#include "utils.h"

class EyeData : public InputData {
public:
    explicit EyeData(){
        timestamp = 0;
        input = cv::Mat();
		pupil = Pupil();
        validPupil = false;
        processingTimestamp = 0;
    }

    cv::Mat input;
	Pupil pupil;
	bool validPupil;
	cv::Rect coarseROI;

    // TODO: header, toQString, and the reading from file (see the Calibration class) should be unified
    // to avoid placing things in the wrong order / with the wrong string
    QString header(QString prefix = "") const {
        QString tmp;
        tmp.append(prefix + "timestamp");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "pupil.x");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "pupil.y");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "pupil.width");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "pupil.height");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "pupil.angle");
        tmp.append(gDataSeparator);
		tmp.append(prefix + "pupil.confidence");
		tmp.append(gDataSeparator);
		tmp.append(prefix + "pupil.valid");
        tmp.append(gDataSeparator);
		tmp.append(prefix + "processingTime");
        tmp.append(gDataSeparator);
        return tmp;
    }

    QString toQString() const {
        QString tmp;
        tmp.append(QString::number(timestamp));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(pupil.center.x));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(pupil.center.y));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(pupil.size.width));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(pupil.size.height));
        tmp.append(gDataSeparator);
		tmp.append(QString::number(pupil.angle));
		tmp.append(gDataSeparator);
		tmp.append(QString::number(pupil.confidence));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(validPupil));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(processingTimestamp));
        tmp.append(gDataSeparator);
        return tmp;
    }
};

Q_DECLARE_METATYPE(EyeData);


class EyeImageProcessorConfig
{
public:
    EyeImageProcessorConfig()
        :
          inputSize(cv::Size(0, 0)),
          flip(CV_FLIP_NONE),
		  undistort(false),
		  coarseDetection(true),
          processingDownscalingFactor(2),
		  pupilDetectionMethod(PuRe::desc.c_str()),
		  tracking(true)
    {}

    cv::Size inputSize;
    CVFlip flip;
	bool undistort;
	bool coarseDetection;
    double processingDownscalingFactor;
	QString pupilDetectionMethod;
	bool tracking;

    void save(QSettings *settings)
    {
        settings->sync();
        settings->setValue("width", inputSize.width);
        settings->setValue("height", inputSize.height);
        settings->setValue("flip", flip);
        settings->setValue("undistort", undistort);
		settings->setValue("coarseDetection", coarseDetection);
		settings->setValue("processingDownscalingFactor", processingDownscalingFactor);
        settings->setValue("pupilDetectionMethod", pupilDetectionMethod);
		settings->setValue("tracking", tracking);
	}

    void load(QSettings *settings)
    {
        settings->sync();
        set(settings, "width", inputSize.width);
        set(settings, "height", inputSize.height);
        set(settings, "flip", flip);
        set(settings, "undistort", undistort);
		set(settings, "coarseDetection", coarseDetection);
		set(settings, "processingDownscalingFactor", processingDownscalingFactor);
        set(settings, "pupilDetectionMethod", pupilDetectionMethod);
		set(settings, "tracking", tracking);
	}
};

class EyeImageProcessorUI : public QDialog
{
    Q_OBJECT

public:
    EyeImageProcessorUI(QWidget *parent=0)
        : QDialog(parent)
    {
        this->setWindowModality(Qt::ApplicationModal);
        this->setWindowTitle("Eye Image Processor Options");
        QGridLayout *layout = new QGridLayout();

        QHBoxLayout *hBoxLayout;
        QFormLayout *formLayout;
        QGroupBox *box;

        box = new QGroupBox("Preprocessing");
        formLayout = new QFormLayout();
        widthSB = new QSpinBox();
        widthSB->setMaximum(4000);
        widthSB->setSuffix(" px");
        widthSB->setWhatsThis("Resizes the input width.\nThe resulting video will have this resolution.\nSetting to 0 disables resizing.");
        widthSB->setToolTip(box->whatsThis());
        formLayout->addRow( new QLabel("Width:"), widthSB );
        heightSB = new QSpinBox();
        heightSB->setMaximum(4000);
        heightSB->setSuffix(" px");
        heightSB->setWhatsThis("Resizes the input height.\nThe resulting video will have this resolution.\nSetting to 0 disables resizing.");
        heightSB->setToolTip(box->whatsThis());
        formLayout->addRow( new QLabel("Height:"), heightSB );
        flipComboBox = new QComboBox();
        flipComboBox->addItem("None", CV_FLIP_NONE);
        flipComboBox->addItem("Horizontal", CV_FLIP_HORIZONTAL);
        flipComboBox->addItem("Vertical", CV_FLIP_VERTICAL);
        flipComboBox->addItem("Both", CV_FLIP_BOTH);
        flipComboBox->setWhatsThis("Flips the input image.");
        flipComboBox->setToolTip(box->whatsThis());
        formLayout->addRow(new QLabel("Flip:"), flipComboBox);
        undistortBox = new QCheckBox();
        undistortBox->setWhatsThis("Undistorsts the input image.\nThe resulting video will be undistorted.\nNot recommended unless using homography gaze estimation.");
        undistortBox->setToolTip(box->whatsThis());
        undistortBox->setEnabled(false);
        formLayout->addRow( new QLabel("Undistort:"), undistortBox );
        box->setLayout(formLayout);
        layout->addWidget(box);

        downscalingSB = new QDoubleSpinBox();
        downscalingSB->setMinimum(1);
        downscalingSB->setSingleStep(1);
        hBoxLayout = new QHBoxLayout();
        box = new QGroupBox("Processing");
        box->setWhatsThis("Processing settings.\nNote: won't affect the recorded video (e.g., downscaling).");
        box->setToolTip(box->whatsThis());
        box->setLayout(hBoxLayout);
        hBoxLayout->addWidget(new QLabel("Downscaling Factor:"));
        hBoxLayout->addWidget(downscalingSB);
        layout->addWidget(box);


		formLayout = new QFormLayout();
		box = new QGroupBox("Pupil Detection");
		box->setWhatsThis("Selects pupil detection method.");
        box->setToolTip(box->whatsThis());
		box->setLayout(formLayout);
		coarseDetectionBox = new QCheckBox();
		coarseDetectionBox->setWhatsThis("Estimate a coarse location for the pupil location prior to detection.");
		coarseDetectionBox->setToolTip(box->whatsThis());
		formLayout->addRow( new QLabel("Coarse Detection:"), coarseDetectionBox );
		pupilDetectionComboBox = new QComboBox();
		formLayout->addRow(pupilDetectionComboBox);
		trackingBox = new QCheckBox();
		trackingBox->setWhatsThis("Track the pupil after detection.");
		trackingBox->setToolTip(box->whatsThis());
		formLayout->addRow( new QLabel("Tracking:"), trackingBox );
		layout->addWidget(box);

        applyButton = new QPushButton("Apply");
        applyButton->setWhatsThis("Applies current configuration.");
        applyButton->setToolTip(applyButton->whatsThis());
        layout->addWidget(applyButton);
        setLayout(layout);
        connect(applyButton, SIGNAL(clicked(bool)),
                this, SLOT(applyConfig()) );
    }
    QSettings *settings;
    QComboBox *pupilDetectionComboBox;

signals:
	void updateConfig();

public slots:
    void showOptions(QPoint pos)
    {
        EyeImageProcessorConfig cfg;
        cfg.load(settings);

        widthSB->setValue(cfg.inputSize.width);
        heightSB->setValue(cfg.inputSize.height);
		downscalingSB->setValue(cfg.processingDownscalingFactor);
		coarseDetectionBox->setChecked(cfg.coarseDetection);
        for (int i=0; i<flipComboBox->count(); i++)
            if (flipComboBox->itemData(i).toInt() == cfg.flip)
                flipComboBox->setCurrentIndex(i);
        for (int i=0; i<pupilDetectionComboBox->count(); i++)
            if (pupilDetectionComboBox->itemData(i).toString() == cfg.pupilDetectionMethod)
                pupilDetectionComboBox->setCurrentIndex(i);
		trackingBox->setChecked(cfg.tracking);
		move(pos);
        show();
    }
    void applyConfig()
    {
        EyeImageProcessorConfig cfg;
        cfg.inputSize.width = widthSB->value();
        cfg.inputSize.height = heightSB->value();
		cfg.processingDownscalingFactor = downscalingSB->value();
		cfg.flip = (CVFlip) flipComboBox->currentData().toInt();
		cfg.coarseDetection = coarseDetectionBox->isChecked();
        cfg.pupilDetectionMethod = pupilDetectionComboBox->currentData().toString();
		cfg.tracking = trackingBox->isChecked();
		cfg.save(settings);
        emit updateConfig();
	}

private:
    QPushButton *applyButton;
    QSpinBox *widthSB, *heightSB;
	QCheckBox *undistortBox;
	QCheckBox *coarseDetectionBox;
	QComboBox *flipComboBox;
	QDoubleSpinBox *downscalingSB;
	QCheckBox *trackingBox;
};

class EyeImageProcessor : public QObject
{
    Q_OBJECT
public:
    explicit EyeImageProcessor(QString id, QObject *parent = 0);
    ~EyeImageProcessor();
    QSettings *settings;
    QVector<PupilDetectionMethod*> availablePupilDetectionMethods;
	EyeImageProcessorConfig cfg;
	EyeData data;

signals:
    void newData(EyeData data);

public slots:
	void process(Timestamp t, const cv::Mat &frame);
	void updateConfig();
	void newROI(QPointF sROI, QPointF eROI);

private:
    QString id;
	QMutex cfgMutex;
    QPointF sROI, eROI;

    PupilDetectionMethod *pupilDetectionMethod;
	PupilTrackingMethod *pupilTrackingMethod;

	unsigned int pmIdx;
};

#endif // EYEIMAGEPROCESSOR_H
