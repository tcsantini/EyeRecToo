#ifndef CAMERA_H
#define CAMERA_H

#include <QObject>
#include <QThread>
#include <QCameraInfo>
#include <QSettings>

#include <QApplication>
#include <QDialog>
#include <QGroupBox>
#include <QComboBox>
#include <QLabel>
#include <QGridLayout>
#include <QRegularExpression>

#include "opencv/cv.h"

#include "FrameGrabber.h"

class CameraUI : public QDialog
{
    Q_OBJECT

public:
    CameraUI(QWidget *parent=0)
        : QDialog(parent)
    {
        this->setWindowModality(Qt::ApplicationModal);
        this->setWindowTitle("Camera Options");
        QGridLayout *layout = new QGridLayout();

        QHBoxLayout *hBoxLayout;
        QGroupBox *box;

        devicesBox = new QComboBox();
        hBoxLayout = new QHBoxLayout();
        box = new QGroupBox("Device:");
        box->setLayout(hBoxLayout);
        hBoxLayout->addWidget(devicesBox);
        layout->addWidget(box);

        settingsBox = new QComboBox();
        hBoxLayout = new QHBoxLayout();
        box = new QGroupBox("Format:");
        box->setLayout(hBoxLayout);
        hBoxLayout->addWidget(settingsBox);
        layout->addWidget(box);

        colorBox = new QComboBox();
        colorBox->addItem("BGR", QVariant(CV_8UC3));
        colorBox->addItem("Gray", QVariant(CV_8UC1));
        hBoxLayout = new QHBoxLayout();
        box = new QGroupBox("Color:");
        box->setLayout(hBoxLayout);
        hBoxLayout->addWidget(colorBox);
        layout->addWidget(box);

        setLayout(layout);
        connect(devicesBox, SIGNAL(currentIndexChanged(int)),
                this, SLOT(deviceChanged(int)) );
        connect(settingsBox, SIGNAL(currentIndexChanged(int)),
                this, SLOT(settingsChanged(int)) );
        connect(colorBox, SIGNAL(currentIndexChanged(int)),
                this, SLOT(colorChanged(int)) );
    }

public slots:
    void update(QCameraInfo current, int colorCode)
    {
        QSignalBlocker blocker(this);
        devicesBox->clear();

        QVariant v;

        QCameraInfo dummy;
        v.setValue(dummy);
        devicesBox->addItem("None", v);

        QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
        int curIdx = 0;
        for (int i=0; i<cameras.size(); i++) {
            v.setValue(cameras[i]);
            devicesBox->addItem(cameras[i].description(), v);
            if (cameras[i] == current)
                curIdx = i + 1; // remember we have none as the first option
        }
        devicesBox->setCurrentIndex(curIdx);

        for (int i=0; i<colorBox->count(); i++)
            if (colorBox->itemData(i).toInt() == colorCode)
                colorBox->setCurrentIndex(i);
    }

    void updateSettings(QList<QCameraViewfinderSettings> settingsList, QCameraViewfinderSettings current)
    {
        QSignalBlocker blocker(this);
        settingsBox->clear();
        QVariant v;

        int curIdx = 0;
        for (int i=0; i<settingsList.size(); i++) {
            // TODO: check with frame grabber
            if (settingsList[i].pixelFormat() != QVideoFrame::Format_Jpeg
                    && settingsList[i].pixelFormat() != QVideoFrame::Format_RGB32
                    && settingsList[i].pixelFormat() != QVideoFrame::Format_YUYV
                    && settingsList[i].pixelFormat() != QVideoFrame::Format_RGB24)
                continue;
            v.setValue(settingsList[i]);
            settingsBox->addItem(toQString(settingsList[i]), v);
            if (settingsList[i] == current)
                curIdx = i;
        }
        settingsBox->setCurrentIndex(curIdx);
    }

signals:
    void setCamera(QCameraInfo cameraInfo);
    void setViewfinderSettings(QCameraViewfinderSettings settings);
    void setColorCode(int code);

private slots:
    void deviceChanged(int i) { emit setCamera(devicesBox->itemData(i).value<QCameraInfo>());}
    void settingsChanged(int i) { emit setViewfinderSettings(settingsBox->itemData(i).value<QCameraViewfinderSettings>());}
    void colorChanged(int i) { emit setColorCode(colorBox->itemData(i).value<int>()); }

private:
    QComboBox *devicesBox;
    QComboBox *settingsBox;
    QComboBox *colorBox;
};

class Camera : public QObject
{
    Q_OBJECT
public:
    explicit Camera(QString id, QObject *parent = 0);
    ~Camera();
    QCameraInfo currentCameraInfo;
    QCameraViewfinderSettings currentViewfinderSettings;
    QCamera *camera;
    CameraUI *ui;
    double fps;

signals:
    void newFrame(Timestamp t, cv::Mat frame);
    void cameraChanged(QCameraInfo currentCamera);
    void noCamera(QString msg);

public slots:
    void setViewfinderSettings(QCameraViewfinderSettings settings);
    void setCamera(const QCameraInfo &cameraInfo);
    void setCamera(const QCameraInfo &cameraInfo, QCameraViewfinderSettings settings);
    void setColorCode(int code);
    void showOptions();
    void saveCfg();
    void loadCfg();
    void timedout();
    void retry();

private:
    QString id;
    int colorCode;

    FrameGrabber *frameGrabber;

    QCameraViewfinderSettings getViewfinderSettings(const QCameraInfo cameraInfo);

    QSettings *settings;

    int retriesLeft;
    int maxRetries;

	static QMutex setCameraMutex;
	void searchDefaultCamera();

private slots:
	void reset();
};

#endif // CAMERA_H
