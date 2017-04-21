#include "FrameGrabber.h"

using namespace std;
using namespace cv;

FrameGrabber::FrameGrabber(QString id, int code, QObject *parent) :
    QAbstractVideoSurface(parent),
    id(id),
    code(code),
    yuvBuffer(NULL),
    yuvBufferSize(0),
    timeoutMs(2000)
{
#ifdef TURBOJPEG
    tjh = tjInitDecompress();
#endif
    watchdog = new QTimer(this);
    connect(watchdog, SIGNAL(timeout()), this, SIGNAL(timedout()));
    watchdog->start(timeoutMs);
}

FrameGrabber::~FrameGrabber()
{
    watchdog->stop();
    watchdog->deleteLater();
#ifdef TURBOJPEG
    tjDestroy(tjh);
#endif
}

QList<QVideoFrame::PixelFormat> FrameGrabber::supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType) const
{
    Q_UNUSED(handleType);
    return QList<QVideoFrame::PixelFormat>()
            << QVideoFrame::Format_Jpeg
            << QVideoFrame::Format_RGB32
            << QVideoFrame::Format_YUYV
            << QVideoFrame::Format_RGB24;
            //<< QVideoFrame::Format_ARGB32_Premultiplied
            //<< QVideoFrame::Format_RGB565
            //<< QVideoFrame::Format_RGB555
            //<< QVideoFrame::Format_ARGB8565_Premultiplied
            //<< QVideoFrame::Format_BGRA32
            //<< QVideoFrame::Format_BGRA32_Premultiplied
            //<< QVideoFrame::Format_BGR24
            //<< QVideoFrame::Format_BGR565
            //<< QVideoFrame::Format_BGR555
            //<< QVideoFrame::Format_BGRA5658_Premultiplied
            //<< QVideoFrame::Format_AYUV444
            //<< QVideoFrame::Format_AYUV444_Premultiplied
            //<< QVideoFrame::Format_YUV444
            //<< QVideoFrame::Format_YUV420P
            //<< QVideoFrame::Format_YV12
            //<< QVideoFrame::Format_UYVY
            //<< QVideoFrame::Format_YUYV
            //<< QVideoFrame::Format_NV12
            //<< QVideoFrame::Format_NV21
            //<< QVideoFrame::Format_IMC1
            //<< QVideoFrame::Format_IMC2
            //<< QVideoFrame::Format_IMC3
            //<< QVideoFrame::Format_IMC4
            //<< QVideoFrame::Format_Y8
            //<< QVideoFrame::Format_Y16
            //<< QVideoFrame::Format_CameraRaw
            //<< QVideoFrame::Format_AdobeDng;
}

bool FrameGrabber::present(const QVideoFrame &frame)
{
    /*
     * IMPORTANT:
     *
     * This frame's data lifetime is not guaranteed once we leave this function
     * so it shouldn't be used outside.
     * If sending the data somewhere else (e.g., for the preview) is necessary,
     * we must copy the data
     *
     */
    Timestamp t = gTimer.elapsed();
    if ( frame.metaData("latency").isValid() ) // uvcengine supports buffering with predefined maximum latency
        t -= frame.metaData("latency").toInt();

    if (!frame.isValid())
        return false;

    QVideoFrame copy(frame);
    Mat cvFrame;

    copy.map(QAbstractVideoBuffer::ReadOnly);
    bool success = false;
    switch (frame.pixelFormat()) {
        case QVideoFrame::Format_Jpeg:
            success = jpeg2bmp(copy, cvFrame);
            break;
        case QVideoFrame::Format_RGB32:
            success = rgb32_2bmp(copy, cvFrame);
        default:
            break;
    }
    copy.unmap();

    if (success && !cvFrame.empty()) {
        watchdog->start(timeoutMs);
        emit newFrame(t, cvFrame);
    }

    return success;
}

void FrameGrabber::setColorCode(int code)
{
    this->code = code;
}

bool FrameGrabber::jpeg2bmp(const QVideoFrame &in, cv::Mat &cvFrame)
{
    unsigned char *frame = const_cast<unsigned char*>(in.bits());
    int len = in.mappedBytes();

#ifdef TURBOJPEG
    int width, height, subsamp, res;
    res = tjDecompressHeader2( tjh, frame, len, &width, &height, &subsamp);
    if (res < 0)
    {
        qWarning() << QString("Frame drop; invalid header: ").append(tjGetErrorStr());
        return false;
    }

    long unsigned int bufSize = tjBufSizeYUV(width, height, subsamp);
    if (bufSize != yuvBufferSize)
    {
        //qInfo() << "YUV buffer size changed";
        yuvBufferSize = bufSize;
        delete yuvBuffer;
        yuvBuffer = new unsigned char[yuvBufferSize];
    }

    res = tjDecompressToYUV( tjh, frame, len, yuvBuffer, 0);
    if (res < 0)
    {
        qWarning() << QString("Frame drop; failed to decompress: ").append(tjGetErrorStr());
        return false;
    }

    cvFrame = Mat::zeros(height, width, code);
    int decode = code == CV_8UC3 ? TJPF_BGR : TJPF_GRAY;
    res = tjDecodeYUV(tjh, yuvBuffer, 4, subsamp, cvFrame.data, width, 0, height, decode, 0);
    if (res < 0)
    {
        qWarning() << QString("Frame drop; failed to decode: ").append(tjGetErrorStr());
        return false;
    }
#else
    std::vector<char> data(frame, frame+len);
    if (code == CV_8U)
        cvFrame = imdecode(Mat(data), CV_LOAD_IMAGE_GRAYSCALE);
    else
        cvFrame = imdecode(Mat(data), CV_LOAD_IMAGE_COLOR);
#endif

    return true;
}

bool FrameGrabber::rgb32_2bmp(const QVideoFrame &in, cv::Mat &cvFrame)
{
    // why abs? Some cameras seem to report some negative frame sizes for DirectShow; I'm looking at you Grasshopper!
    Mat rgba = Mat(abs(in.height()), abs(in.width()), CV_8UC4, (void*) in.bits());
    if (code == CV_8UC3)
        cvtColor(rgba, cvFrame, CV_BGRA2BGR);
    else
        cvtColor(rgba, cvFrame, CV_BGRA2GRAY);
    return true;
}

//TODO: add support for other frame formats
