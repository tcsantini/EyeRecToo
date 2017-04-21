#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <atomic>

#ifndef M_PI
#define M_PI 	 3.14159265358979323846
#endif

#include <QElapsedTimer>
#include <QDebug>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QCameraViewfinderSettings>
#include <QCameraInfo>
#include <QWidget>
#include <QSettings>

#include <opencv/cv.h>
#include <opencv2/imgproc.hpp>
#include <opencv2/calib3d.hpp>

/*
 * Global variables
 */
extern QElapsedTimer gTimer;

typedef qint64 Timestamp;
extern Timestamp maxTimestamp;

extern QString gExeDir;
extern QString gCfgDir;
extern char gDataSeparator;
extern char gDataNewline;
extern std::atomic<bool> gRecording;
extern bool gHasOpenH264;

extern QList<QPair<QString, QString> > gReferenceList;

/*
 * Utility functions
 */

QDebug operator<<(QDebug dbg, const cv::Point &p);

QDebug operator<<(QDebug dbg, const cv::Point2f &p);

QDebug operator<<(QDebug dbg, const cv::Point3f &p);

QDebug operator<<(QDebug dbg, const QCameraViewfinderSettings &p);

template<typename T> double ED(T p1, T p2) { return sqrt( (p1.x-p2.x) * (p1.x-p2.x) + (p1.y-p2.y) * (p1.y-p2.y) ); }


cv::Point3f estimateMarkerCenter(const std::vector<cv::Point2f> corners);

void logInitBanner();
void logExitBanner();
void logMessages(QtMsgType type, const QMessageLogContext &context, const QString &msg);

// Converstion
cv::Point2f to2D(const cv::Point3f p);
cv::Point3f to3D(const cv::Point2f p);

QString toQString(QVideoFrame::PixelFormat format);
QString toQString(QCameraViewfinderSettings setting);

QString iniStr(QString str);

void delay(int thMs);

enum CVFlip { CV_FLIP_BOTH = -1, CV_FLIP_VERTICAL = 0, CV_FLIP_HORIZONTAL = 1, CV_FLIP_NONE = 2};
Q_DECLARE_METATYPE(enum CVFlip)

#define CV_BLUE 	cv::Scalar(0xff,0xb0,0x00)
#define CV_GREEN 	cv::Scalar(0x03,0xff,0x76)
#define CV_RED 		cv::Scalar(0x00,0x3d,0xff)
#define CV_YELLOW	cv::Scalar(0x00,0xea,0xff)
#define CV_CYAN		cv::Scalar(0xff,0xff,0x18)
#define CV_MAGENT   cv::Scalar(0x81,0x40,0xff)
#define CV_WHITE	cv::Scalar(0xff,0xff,0xff)
#define CV_BLACK	cv::Scalar(0x00,0x00,0x00)

template<typename T> void set(const QSettings *settings, const QString key, T &v)
{
    if (!settings)
        return;
    QVariant variant = settings->value(key);
    if (variant.isValid())
        v = qvariant_cast<T>(variant);
}

#endif // UTILS_H
