#include "Utils.h"

/*
 * Global variables
 */

QElapsedTimer gTimer;

Timestamp maxTimestamp = std::numeric_limits<qint64>::max();

QString gExeDir;
QString gCfgDir;

char gDataSeparator = '\t';
char gDataNewline = '\n';

QMutex gLogMutex;
QFile gLogFile;
QTextStream gLogStream;
std::atomic<bool> gRecording(false);
bool gHasOpenH264 = false;

/*
 * Utility functions
 */

QDebug operator<<(QDebug dbg, const cv::Point &p)
{
    dbg.nospace() << "(" << p.x << ", " << p.y << ")";
    return dbg.space();
}

QDebug operator<<(QDebug dbg, const cv::Point2f &p)
{
    dbg.nospace() << "(" << p.x << ", " << p.y << ")";
    return dbg.space();
}

QDebug operator<<(QDebug dbg, const cv::Point3f &p)
{
    dbg.nospace() << "(" << p.x << ", " << p.y << ", " << p.z << ")";
    return dbg.space();
}

QDebug operator<<(QDebug dbg, const QCameraViewfinderSettings &p)
{
    dbg.nospace() << toQString(p);
    return dbg.space();
}

void logInitBanner()
{
    if (!gLogFile.isOpen()) {
        gLogFile.setFileName("EyeRecToo.log");
        gLogFile.open(QIODevice::WriteOnly|QIODevice::Append);
        gLogStream.setDevice(&gLogFile);
    }

    QDateTime utc = QDateTime::currentDateTimeUtc();
    qDebug() << "Starting\n######################################################################"
             << "\n# " << QString("EyeRecToo v%1").arg(GIT_VERSION)
             << "\n# UTC:      " << utc.toString()
             << "\n# Local:    " << utc.toLocalTime().toString()
             << "\n######################################################################";
}

void logExitBanner()
{
    QDateTime utc = QDateTime::currentDateTimeUtc();
    qDebug() << "Exiting\n######################################################################"
             << "\n# UTC:      " << utc.toString()
             << "\n# Local:    " << utc.toLocalTime().toString()
             << "\n######################################################################\n";

    std::cout.flush();
    gLogStream.flush();
}

void logMessages(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QMutexLocker lock(&gLogMutex);

    std::string str = qFormatLogMessage(type, context, msg).prepend(
                              QString("[%1] ").arg(gTimer.elapsed(), 9, 'f', 0, ' ')
                          ).append("\n").toStdString();

    std::cout << str.c_str();
    if (gLogStream.status() == QTextStream::Ok)
        gLogStream << str.c_str();

    std::cout.flush();
    gLogStream.flush();
}

cv::Point3f estimateMarkerCenter(const std::vector<cv::Point2f> corners)
{
    cv::Point3f cp(0,0,0);
    if (corners.size() != 4)
        return cp;

#ifdef HOMOGRAPHY_ESTIMATION
    // Homography is overkill here
    std::vector<cv::Point2f> plane;
    plane.push_back(cv::Point2f(0,0));
    plane.push_back(cv::Point2f(10,0));
    plane.push_back(cv::Point2f(10,10));
    plane.push_back(cv::Point2f(0,10));

    cv::Mat H = cv::findHomography(plane, corners);
    std::vector<cv::Point2f> planeCenter;
    planeCenter.push_back(cv::Point2f(5,5));
    std::vector<cv::Point2f> markerCenter;
    cv::perspectiveTransform(planeCenter, markerCenter, H);
    cp.x = markerCenter[0].x;
    cp.y = markerCenter[0].y;
#else
    double a1 = 0;
    double b1 = 0;
    double a2 = 0;
    double b2 = 0;

    if ( corners[2].x - corners[0].x != 0)
        a1 = (corners[2].y - corners[0].y) / (corners[2].x - corners[0].x);
    b1 = corners[2].y - corners[2].x*a1;

    if ( corners[3].x - corners[1].x != 0)
        a2 = (corners[3].y - corners[1].y) / (corners[3].x - corners[1].x);
    b2 = corners[3].y - corners[3].x*a2;

    cp.x = (b2-b1) / (a1-a2);
    cp.y = a2*cp.x + b2;
#endif

    return cp;
}

cv::Point2f to2D(const cv::Point3f p) { return cv::Point2f(p.x, p.y); }
cv::Point3f to3D(const cv::Point2f p) { return cv::Point3f(p.x, p.y, 0); }

QString toQString(QVideoFrame::PixelFormat format) {
    switch (format) {
        case QVideoFrame::Format_Jpeg:
            return "JPEG";
        case QVideoFrame::Format_YUYV:
            return "YUYV";
        case QVideoFrame::Format_RGB32:
            return "RGB32";
        case QVideoFrame::Format_RGB24:
            return "RGB24";
        case QVideoFrame::Format_Invalid:
            return "Invalid";
        default:
            return "Unsupported";
    }
}

QString toQString(QCameraViewfinderSettings setting) {
    return QString::number(setting.resolution().width())
                    + " x "
                    + QString::number(setting.resolution().height())
                    + " "
                    + toQString(setting.pixelFormat())
                    + " @ "
                    + QString::number((int)setting.maximumFrameRate()) + " FPS";
}

QString iniStr(QString str)
{
    return str.remove(QRegExp("[^a-zA-Z\\d\\s]"));
}

// CPU hoggers for testing
void delay(int thMs) { Timestamp cur = gTimer.elapsed(); volatile int a=0; while ( gTimer.elapsed() - cur < thMs ) a++; }

void loadSoundEffect(QSoundEffect &effect, QString fileName)
{
    // TODO: clean this up; QSoundEffect doesn't work with resources currently so
    // we search manually in case this is a release or a development environment.
    // It assumes the build location for the development environment!

    QStringList searchPaths;
    searchPaths << "../EyeRecToo/effects" << "./effects";
    for (int i=0; i<searchPaths.size(); i++) {
        QString target = searchPaths[i] + "/" + fileName;
        if (QFile(target).exists()) {
            qDebug() << "Loaded" << target;
            effect.setSource(QUrl::fromLocalFile(target));
            break;
        }
    }
}
