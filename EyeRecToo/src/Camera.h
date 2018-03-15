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
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QFileInfo>

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
		QFormLayout *formLayout;
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

		// Sliders are prettier and easier, but spinboxes are more accurate; unfortunatelly, that's what we favor.
		bool useSliders = false;
		QComboBox *parBox;
		box = new QGroupBox("Parameters:");
		box->setWhatsThis("Set camera parameters mode or values as % of their range.\nCurrently, no test is performed to check if the camera actually supports changing the parameter.");
		box->setToolTip(box->whatsThis());
		formLayout = new QFormLayout();
		QStringList list = {
			"Brightness",
			"Contrast",
			"White Balance",
			"Saturation",
			"Sharpening Level",
			//"Gamma",
			//"Gain",
			//"Backlight",
			"Exposure Time",
		};
		for (auto i = list.begin(); i != list.end(); i++) {
			if (useSliders)
				addSlider(formLayout, *i);
			else
				addSpinBox(formLayout, *i);
		}
		parBox = addComboBox(formLayout, "Exposure Mode");
		parBox->addItem("Manual", 1);
		parBox->addItem("Auto", 2);
		//parBox->addItem("Shutter", 4);
		//parBox->addItem("Aperture", 8);

		box->setLayout(formLayout);
		layout->addWidget(box, 0, 1, 3, 1);

        setLayout(layout);
        connect(devicesBox, SIGNAL(currentIndexChanged(int)),
                this, SLOT(deviceChanged(int)) );
        connect(settingsBox, SIGNAL(currentIndexChanged(int)),
                this, SLOT(settingsChanged(int)) );
        connect(colorBox, SIGNAL(currentIndexChanged(int)),
                this, SLOT(colorChanged(int)) );
	}

	void setValue(QDoubleSpinBox *sb, double val) {
		if (!sb)
			return;
		sb->blockSignals(true);
		sb->setValue(100*val);
		sb->blockSignals(false);
	}
	void setValue(QSlider *s, double val) {
		if (!s)
			return;
		s->blockSignals(true);
		s->setValue(100*val);
		s->blockSignals(false);
	}
	void setValue(QComboBox *cb, QVariant val) {
		if (!cb)
			return;
		cb->blockSignals(true);
		cb->setCurrentIndex(cb->findData(val));
		cb->blockSignals(false);
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
	void setParameter(QString what, float value);

private slots:
    void deviceChanged(int i) { emit setCamera(devicesBox->itemData(i).value<QCameraInfo>());}
    void settingsChanged(int i) { emit setViewfinderSettings(settingsBox->itemData(i).value<QCameraViewfinderSettings>());}
	void colorChanged(int i) { emit setColorCode(colorBox->itemData(i).value<int>()); }
	void sliderReleased() {
		QSlider *slider = static_cast<QSlider*>( QObject::sender() );
		emit setParameter(slider->objectName(), slider->value() / 100.0);
	}
	void spinBoxChanged(double value) {
		QDoubleSpinBox *sb = static_cast<QDoubleSpinBox*>( QObject::sender() );
		emit setParameter(sb->objectName(), value/ 100.0);
	}
	void comboBoxChanged() {
		QComboBox *box = static_cast<QComboBox*>( QObject::sender() );
		emit setParameter(box->objectName(), box->currentData().toFloat());
	}

private:
    QComboBox *devicesBox;
    QComboBox *settingsBox;
    QComboBox *colorBox;

	void addSlider(QFormLayout *formLayout, QString label ) {
		QSlider *slider = new QSlider( Qt::Horizontal );
		slider->setMinimum(0);
		slider->setMaximum(100);
		slider->setSingleStep(1);
		slider->setObjectName(label.toLower().replace(" ", "_"));
		formLayout->addRow(new QLabel(label), slider);
		connect(slider, SIGNAL(sliderReleased()), this, SLOT(sliderReleased()) );
	}

	void addSpinBox(QFormLayout *formLayout, QString label ) {
		QDoubleSpinBox *sb = new QDoubleSpinBox();
		sb->setMinimum(0);
		sb->setMaximum(100);
		sb->setSingleStep(0.5);
		sb->setDecimals(1);
		sb->setObjectName(label.toLower().replace(" ", "_"));
		sb->setSuffix("%");
		formLayout->addRow(new QLabel(label), sb);
		connect(sb, SIGNAL(valueChanged(double)), this, SLOT(spinBoxChanged(double)) );
	}

	QComboBox* addComboBox(QFormLayout *formLayout, QString label ) {
		QComboBox *box = new QComboBox();
		box->setObjectName(label.toLower().replace(" ", "_"));
		formLayout->addRow(new QLabel(label), box);
		connect(box, SIGNAL(currentIndexChanged(int)), this, SLOT(comboBoxChanged()) );
		return box;
	}
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
	void setParameter(QString what, float value);
	void setValuesUI();
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

	QString makeSettingsFileName() { return QString("%1/cfg/camera-parameters/%2.ini").arg(QCoreApplication::applicationDirPath()).arg(currentCameraInfo.deviceName()); }
	bool loadCameraParameter(const QSettings &settings, const QString &parameter, double &value)
	{
		if ( ! settings.contains(parameter) )
			return false;
		value = settings.value(parameter).toDouble();
		return true;
	}
	bool loadCameraParameter(const QString &parameter, double &value)
	{
		QFileInfo cameraSettingsFile( makeSettingsFileName() );
		if ( ! cameraSettingsFile.exists() )
			return false;
		QSettings settings(cameraSettingsFile.absoluteFilePath(), QSettings::IniFormat);
		return loadCameraParameter(settings, parameter, value);
	}
	void saveCameraParameter(const QString &parameter, const double &value)
	{
		QFileInfo cameraSettingsFile( makeSettingsFileName() );
		QSettings s(cameraSettingsFile.absoluteFilePath(), QSettings::IniFormat);
		s.setValue(parameter, value);
	}
	void loadAndSet(const QSettings &settings, QString key) {
		double value;
		if ( loadCameraParameter(settings, key, value) )
			setParameter(key, value);

	}
	void loadUserCameraParameters();

private slots:
	void reset();
};

#endif // CAMERA_H
