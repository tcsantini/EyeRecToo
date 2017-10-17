#ifndef FRAMEGRABBER_H
#define FRAMEGRABBER_H

#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>
#include <QElapsedTimer>
#include <QThread>
#include <QTimer>
#include <QDebug>

#include <opencv/cv.hpp>

#ifdef TURBOJPEG
#include "turbojpeg.h"
#endif

#include "utils.h"

class FrameGrabber : public QAbstractVideoSurface
{
    Q_OBJECT
public:
    explicit FrameGrabber(QString id, int code, QObject *parent = 0);
    ~FrameGrabber();
    QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType) const;

signals:
    void newFrame(Timestamp t, cv::Mat frame);
    void timedout();

public slots:
    bool present(const QVideoFrame &frame);
    void setColorCode(int code);

private:
    QTimer *watchdog;
    int timeoutMs;
    int timestampOffset;
#ifdef TURBOJPEG
    tjhandle tjh;
#endif
    QString id;
    int code;
    unsigned char* yuvBuffer;
    long unsigned int yuvBufferSize;
    bool jpeg2bmp(const QVideoFrame &in, cv::Mat &cvFrame);
    bool rgb32_2bmp(const QVideoFrame &in, cv::Mat &cvFrame);
    bool yuyv_2bmp(const QVideoFrame &in, cv::Mat &cvFrame);

    unsigned int pmIdx;
};

#endif // FRAMEGRABBER_H
