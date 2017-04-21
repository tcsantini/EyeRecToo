#include "Camera.h"

static int gQCameraInfoMetaTypeId = qRegisterMetaType<QCameraInfo>("QCameraInfo");
static int gMatMetaTypeId = qRegisterMetaType<cv::Mat>("cv::Mat");
static int gTimestampMetaTypeId = qRegisterMetaType<Timestamp>("Timestamp");
static int gQListQCameraViewfinderSettingsId = qRegisterMetaType<QList<QCameraViewfinderSettings> >("QList<QCameraViewfinderSettings>");

QMutex Camera::setCameraMutex;

Camera::Camera(QString id, QObject *parent)
    : QObject(parent),
      colorCode(CV_8UC3),
      camera(NULL),
      frameGrabber(NULL)
{
    this->id = id;
    if (id.contains("eye", Qt::CaseInsensitive) )
            colorCode = CV_8UC1;
    ui = new CameraUI();
    ui->moveToThread(QApplication::instance()->thread());
    connect(ui, SIGNAL(setCamera(QCameraInfo)), this, SLOT(setCamera(QCameraInfo)) );
    connect(ui, SIGNAL(setViewfinderSettings(QCameraViewfinderSettings)), this, SLOT(setViewfinderSettings(QCameraViewfinderSettings)) );
    connect(ui, SIGNAL(setColorCode(int)), this, SLOT(setColorCode(int)) );
    settings = new QSettings(gCfgDir + "/" + id + " Camera", QSettings::IniFormat);
}

Camera::~Camera()
{
    reset();
    ui->deleteLater();
    if (settings)
        settings->deleteLater();
}

void Camera::reset()
{
    QList<QCameraViewfinderSettings> settingsList;
    QMetaObject::invokeMethod(ui, "updateSettings", Q_ARG(QList<QCameraViewfinderSettings>, settingsList), Q_ARG(QCameraViewfinderSettings, currentViewfinderSettings) );
    if (camera) {
        camera->stop();
        camera->unload();
        // Delete straight away so the destructor from uvcengine service get's
        // called and the device is not busy anymore
        //camera->deleteLater();
        delete camera;
        camera = NULL;
    }

    if (frameGrabber) {
        frameGrabber->deleteLater();
        frameGrabber = NULL;
    }
}

QCameraViewfinderSettings Camera::getViewfinderSettings(const QCameraInfo cameraInfo)
{
    QCameraViewfinderSettings recommended;

    // Recommend based on known cameras
    QString description = cameraInfo.description();
    if (description == "Pupil Cam1 ID0") {
        recommended.setMaximumFrameRate(120);
        recommended.setMinimumFrameRate(120);
        recommended.setResolution(640, 480);
        recommended.setPixelFormat( QVideoFrame::Format_Jpeg);
    } else if (description == "Pupil Cam1 ID1") {
        recommended.setMaximumFrameRate(120);
        recommended.setMinimumFrameRate(120);
        recommended.setResolution(640, 480);
        recommended.setPixelFormat( QVideoFrame::Format_Jpeg);
    } else if (description == "Pupil Cam1 ID2") {
        recommended.setMaximumFrameRate(30);
        recommended.setMinimumFrameRate(30);
        recommended.setResolution(1280, 720);
        recommended.setPixelFormat( QVideoFrame::Format_Jpeg);
    } else {
        // Unknown; recommend to maximize fps and minimize resolution
        recommended = camera->viewfinderSettings();
        foreach (const QCameraViewfinderSettings &setting,  camera->supportedViewfinderSettings()) {
            if ( setting.pixelFormat() == QVideoFrame::Format_RGB32
                 || setting.pixelFormat() == QVideoFrame::Format_RGB24
                 || setting.pixelFormat() == QVideoFrame::Format_YUYV
                 || setting.pixelFormat() == QVideoFrame::Format_Jpeg ) {

                if (recommended.isNull())
                    recommended = setting;
                else
                    if (setting.maximumFrameRate() >= recommended.maximumFrameRate())
                        if (setting.resolution().width()*setting.resolution().height() < recommended.resolution().width()*recommended.resolution().height())
                            recommended = setting;
            }
        }
        return recommended;
    }

    // Get the exact setting based on known camera recomendation
    foreach (const QCameraViewfinderSettings &setting,  camera->supportedViewfinderSettings()) {
        if (recommended.pixelFormat() != setting.pixelFormat())
            continue;
        if (recommended.resolution() != setting.resolution())
            continue;
        if ( fabs(recommended.maximumFrameRate() - setting.maximumFrameRate()) > 1)
            continue;
        return setting;
    }

    // This shouldn't happen unless the recomendation is wrong
    return camera->viewfinderSettings();
}

void Camera::setViewfinderSettings(QCameraViewfinderSettings settings)
{
    setCamera(currentCameraInfo, settings);
}

void Camera::setCamera(const QCameraInfo &cameraInfo)
{
    QCameraViewfinderSettings settings;
    setCamera(cameraInfo, settings);
}

void Camera::setCamera(const QCameraInfo &cameraInfo, QCameraViewfinderSettings settings)
{
    QMutexLocker setCameraLocker(&setCameraMutex);

    reset();

    QList<QCameraViewfinderSettings> settingsList;
    QString msg = "No camera selected";
    if (cameraInfo.isNull()) {
        currentCameraInfo = QCameraInfo();
        currentViewfinderSettings = QCameraViewfinderSettings();
        emit noCamera(msg);
    } else {

        qInfo() << id << "Opening" << cameraInfo.description();
        camera = new QCamera(cameraInfo.deviceName().toUtf8());
        frameGrabber = new FrameGrabber(id, colorCode);

        camera->load();
        if (camera->state() == QCamera::UnloadedState) {
            qInfo() << id << cameraInfo.description() << "failed to load:\n" << camera->errorString();
            reset();
            emit noCamera("Failed to load.");
            return;
        }

        if (settings.isNull())
            settings = getViewfinderSettings(cameraInfo);

        camera->setViewfinderSettings(settings);
        camera->setViewfinder(frameGrabber);
        camera->start();

        if (camera->state() != QCamera::ActiveState) {
            qInfo() << id << cameraInfo.description() << "failed to start (" << settings << ")\n" << camera->errorString();
            reset();
            emit noCamera("Failed to start.");
            return;
        }

        connect(frameGrabber, SIGNAL(newFrame(Timestamp, cv::Mat)),
                this, SIGNAL(newFrame(Timestamp, cv::Mat)) );
        connect(frameGrabber, SIGNAL(timedout()),
                this, SLOT(timedout()) );

        fps = settings.maximumFrameRate();
        currentCameraInfo = cameraInfo;
        currentViewfinderSettings = settings;
        settingsList = camera->supportedViewfinderSettings();
        msg = currentCameraInfo.description() + " " + toQString(settings);
    }
    saveCfg();
    qInfo() << id << msg;
    QMetaObject::invokeMethod(ui, "updateSettings", Q_ARG(QList<QCameraViewfinderSettings>, settingsList), Q_ARG(QCameraViewfinderSettings, currentViewfinderSettings) );
}

void Camera::setColorCode(int code)
{
    colorCode = code;
    QMetaObject::invokeMethod(frameGrabber, "setColorCode", Q_ARG(int, colorCode));
    saveCfg();
}

void Camera::showOptions()
{
    QMetaObject::invokeMethod(ui, "update", Q_ARG(QCameraInfo, currentCameraInfo), Q_ARG(int, colorCode));
    QMetaObject::invokeMethod(ui, "show");
}

void Camera::saveCfg()
{
    settings->sync();
    settings->setValue("description", iniStr(currentCameraInfo.description()));
    settings->setValue("deviceName", iniStr(currentCameraInfo.deviceName()));
    settings->setValue("width", currentViewfinderSettings.resolution().width());
    settings->setValue("height", currentViewfinderSettings.resolution().height());
    settings->setValue("fps", currentViewfinderSettings.maximumFrameRate());
    settings->setValue("format", currentViewfinderSettings.pixelFormat());
    settings->setValue("wPxRatio", currentViewfinderSettings.pixelAspectRatio().width());
    settings->setValue("hPxRatio", currentViewfinderSettings.pixelAspectRatio().height());
    settings->setValue("colorCode", colorCode);
}

void Camera::loadCfg()
{
    settings->sync();
    QString description;
    set(settings, "description", description);
    QString deviceName;
    set(settings, "deviceName", deviceName);

    QCameraInfo info;
    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    for (int i=0; i<cameras.size(); i++) {
        if (iniStr(cameras[i].description()) == description && iniStr(cameras[i].deviceName()) == deviceName)
            info = cameras[i];
    }

    QCameraViewfinderSettings viewFinderSetting;

    double fps = 30;
    set(settings, "fps", fps);
    viewFinderSetting.setMaximumFrameRate(fps);
    viewFinderSetting.setMinimumFrameRate(fps);

    double width = 640;
    double height = 480;
    set(settings, "width", width);
    set(settings, "height", height);
    viewFinderSetting.setResolution( width, height );

    double wPxRatio = 1;
    double hPxRatio = 1;
    set(settings, "wPxRatio", wPxRatio);
    set(settings, "hPxRatio", hPxRatio);
    viewFinderSetting.setPixelAspectRatio( wPxRatio, hPxRatio );

    QVideoFrame::PixelFormat format = QVideoFrame::Format_BGR24;
    set(settings, "format", format);
    viewFinderSetting.setPixelFormat( format );

    set(settings, "colorCode", colorCode);

    setCamera(info, viewFinderSetting);
}

void Camera::timedout()
{
    qWarning() << id << "timedout; reopening...";
    int retries = 15;

    QCameraInfo cameraInfo = currentCameraInfo;
    QCameraViewfinderSettings viewfinderSettings = currentViewfinderSettings;

    currentCameraInfo = QCameraInfo();
    currentViewfinderSettings = QCameraViewfinderSettings();

    for (int i=1; i<=retries; i++) {
        setCamera(cameraInfo, viewfinderSettings);
        if (!currentCameraInfo.isNull())
            break;
        QThread::msleep(1000);
    }
}
