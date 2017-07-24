#ifndef FIELDIMAGEPROCESSOR_H
#define FIELDIMAGEPROCESSOR_H

#include <vector>

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
#include <opencv2/aruco.hpp>
#include <opencv2/aruco/charuco.hpp>

#include "InputWidget.h"

#include "utils.h"

class Marker {
public:
    explicit Marker() :
        corners(std::vector<cv::Point2f>()),
        center(cv::Point3f(0,0,0)),
        id(-1),
        rv(cv::Mat()),
        tv(cv::Mat()) { }
    explicit Marker(std::vector<cv::Point2f> corners, int id) :
        corners(corners),
        center( cv::Point3f(0,0,0) ),
        id(id),
        rv(cv::Mat()),
        tv(cv::Mat()) { }
    std::vector<cv::Point2f> corners;
    cv::Point3f center;
    // Not exported atm
    int id;
    cv::Mat rv;
    cv::Mat tv;
};

class FieldData : public InputData {
public:
    explicit FieldData(){
        timestamp = 0;
        input = cv::Mat();
        gazeEstimate = cv::Point3f(0,0,0);
        validGazeEstimate = false;
        extrapolatedGazeEstimate = 0;
        markers = std::vector<Marker>();
        collectionMarker = Marker();
        cameraMatrix = cv::Mat();
        distCoeffs = cv::Mat();
        undistorted = false;
        width = 0;
        height = 0;
        processingTimestamp = 0;
    }
    cv::Mat input;
    cv::Point3f gazeEstimate;
    bool validGazeEstimate;
    int extrapolatedGazeEstimate;
    Marker collectionMarker;
    std::vector<Marker> markers;
    cv::Mat cameraMatrix, distCoeffs;
    bool undistorted;
    unsigned int width;
    unsigned int height;

    QString header(QString prefix = "") const {
        QString tmp;
        tmp.append(prefix + "timestamp");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "gaze.x");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "gaze.y");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "gaze.z");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "gaze.valid");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "collectionMarker.id");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "collectionMarker.x");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "collectionMarker.y");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "collectionMarker.z");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "undistorted");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "width");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "height");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "markers");
        tmp.append(gDataSeparator);
        tmp.append(prefix + "processingTimestamp");
        tmp.append(gDataSeparator);
        return tmp;
    }

    QString toQString() const {
        QString tmp;
        tmp.append(QString::number(timestamp));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(gazeEstimate.x));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(gazeEstimate.y));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(gazeEstimate.z));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(validGazeEstimate));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(collectionMarker.id));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(collectionMarker.center.x));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(collectionMarker.center.y));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(collectionMarker.center.z));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(undistorted));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(width));
        tmp.append(gDataSeparator);
        tmp.append(QString::number(height));
        tmp.append(gDataSeparator);
        for ( unsigned int i=0; i<markers.size(); i++) {
            tmp.append(QString::number(markers[i].id));
            tmp.append(":");
            tmp.append(QString::number(markers[i].center.x));
            tmp.append("x");
            tmp.append(QString::number(markers[i].center.y));
            tmp.append("x");
            tmp.append(QString::number(markers[i].center.z));
            tmp.append(";");
        }
        tmp.append(gDataSeparator);
        tmp.append(QString::number(processingTimestamp));
        tmp.append(gDataSeparator);
        return tmp;
    }
};

Q_DECLARE_METATYPE(FieldData)

class FieldImageProcessorConfig
{
public:
    FieldImageProcessorConfig()
        :
          inputSize(cv::Size(0, 0)),
          flip(CV_FLIP_NONE),
          collectionMarkerId(128),
          collectionMarkerSizeMeters(0.10),
          processingDownscalingFactor(2),
          undistort(false),
          markerDetectionMethod("aruco")
    {
    }

    cv::Size inputSize;
    CVFlip flip;
    int collectionMarkerId;
    double collectionMarkerSizeMeters;
    double processingDownscalingFactor;
    bool undistort;
    QString markerDetectionMethod;

    void save(QSettings *settings)
    {
        settings->sync();
        settings->setValue("width", inputSize.width);
        settings->setValue("height", inputSize.height);
        settings->setValue("flip", flip);
        settings->setValue("collectionMarkerId", collectionMarkerId);
        settings->setValue("collectionMarkerSizeMeters", collectionMarkerSizeMeters);
        settings->setValue("processingDownscalingFactor", processingDownscalingFactor);
        settings->setValue("undistort", undistort);
        settings->setValue("markerDetectionMethod", markerDetectionMethod);
    }

    void load(QSettings *settings)
    {
        settings->sync();
        set(settings, "width", inputSize.width);
        set(settings, "height", inputSize.height);
        set(settings, "flip", flip);
        set(settings, "collectionMarkerId", collectionMarkerId);
        set(settings, "collectionMarkerSizeMeters", collectionMarkerSizeMeters);
        set(settings, "processingDownscalingFactor", processingDownscalingFactor);
        set(settings, "undistort", undistort);
        set(settings, "markerDetectionMethod", markerDetectionMethod);
    }
};

class FieldImageProcessorUI : public QDialog
{
    Q_OBJECT

public:
    FieldImageProcessorUI(QWidget *parent=0)
        : QDialog(parent)
    {
        this->setWindowModality(Qt::ApplicationModal);
        this->setWindowTitle("Field Image Processor Options");
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

        hBoxLayout = new QHBoxLayout();
        box = new QGroupBox("Marker Detection");
        box->setWhatsThis("Selects marker detection method.");
        box->setToolTip(box->whatsThis());
        box->setLayout(hBoxLayout);
        markerDetectionComboBox = new QComboBox();
        markerDetectionComboBox->addItem("None", "None");
        markerDetectionComboBox->addItem("ArUcO (Garrido-Jurado et al. 2014)", "aruco");
        hBoxLayout->addWidget(markerDetectionComboBox);
        layout->addWidget(box);

        cmIdSB = new QSpinBox();
        cmIdSB->setMaximum(1000);
        cmSizeSB = new QDoubleSpinBox();
        cmSizeSB->setMaximum(100);
        cmSizeSB->setSingleStep(0.01);
        cmSizeSB->setSuffix(" m");
        hBoxLayout = new QHBoxLayout();
        box = new QGroupBox("Collection Marker");
        box->setWhatsThis("Collection marker settings.\nConfigures the ID and the size of the collection marker employed by CalibMe.");
        box->setToolTip(box->whatsThis());
        box->setLayout(hBoxLayout);
        hBoxLayout->addWidget(new QLabel("ID:"));
        hBoxLayout->addWidget(cmIdSB);
        hBoxLayout->addWidget(new QLabel("Size:"));
        hBoxLayout->addWidget(cmSizeSB);
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

signals:
    void updateConfig();

public slots:
    void showOptions(QPoint pos)
    {
        FieldImageProcessorConfig cfg;
        cfg.load(settings);

        widthSB->setValue(cfg.inputSize.width);
        heightSB->setValue(cfg.inputSize.height);
        for (int i=0; i<flipComboBox->count(); i++)
            if (flipComboBox->itemData(i).toInt() == cfg.flip)
                flipComboBox->setCurrentIndex(i);
        undistortBox->setChecked(cfg.undistort);
        cmIdSB->setValue(cfg.collectionMarkerId);
        cmSizeSB->setValue(cfg.collectionMarkerSizeMeters);
        downscalingSB->setValue(cfg.processingDownscalingFactor);
        for (int i=0; i<markerDetectionComboBox->count(); i++)
            if (markerDetectionComboBox->itemData(i).toString() == cfg.markerDetectionMethod)
                markerDetectionComboBox->setCurrentIndex(i);
        move(pos);
        show();
    }
    void applyConfig()
    {
        FieldImageProcessorConfig cfg;
        cfg.inputSize.width = widthSB->value();
        cfg.inputSize.height = heightSB->value();
        cfg.flip = (CVFlip) flipComboBox->currentData().toInt();
        cfg.undistort = undistortBox->isChecked();
        cfg.collectionMarkerId = cmIdSB->value();
        cfg.collectionMarkerSizeMeters = cmSizeSB->value();
        cfg.processingDownscalingFactor = downscalingSB->value();
        cfg.markerDetectionMethod = markerDetectionComboBox->currentData().toString();
        cfg.save(settings);
        emit updateConfig();
    }

private:
    QPushButton *applyButton;
    QSpinBox *widthSB, *heightSB;
    QComboBox *flipComboBox;
    QCheckBox *undistortBox;
    QSpinBox *cmIdSB;
    QDoubleSpinBox *cmSizeSB;
    QDoubleSpinBox *downscalingSB;
    QComboBox *markerDetectionComboBox;
};

class FieldImageProcessor : public QObject
{
    Q_OBJECT
public:
    explicit FieldImageProcessor(QString id, QObject *parent = 0);
    ~FieldImageProcessor();
    QSettings *settings;

signals:
    void newData(FieldData data);

public slots:
    void process(Timestamp t, const cv::Mat &frame);
    void updateConfig();
    void newROI(QPointF sROI, QPointF eROI);

private:
    QString id;
    FieldImageProcessorConfig cfg;
    QMutex cfgMutex;
    FieldData data;
    QPointF sROI, eROI;

    cv::Ptr<cv::aruco::Dictionary> dict;
    cv::Ptr<cv::aruco::DetectorParameters> detectorParameters;

    // Undistortion
    cv::Mat cameraMatrix, newCameraMatrix, distCoeffs;
    cv::Mat rvecs, tvecs;
    cv::Mat map1, map2;
    cv::Size imageSize, newImageSize;
    bool forceSanitize;
    void sanitizeCameraParameters(cv::Size size);
};

#endif // FIELDIMAGEPROCESSOR_H
