#include "DataRecorder.h"

using namespace cv;

DataRecorder::DataRecorder(QString id, QString header, QObject *parent)
    : id(id),
      header(header),
      videoWriter(NULL),
      dataFile(NULL),
      dataStream(NULL),
      framerate(0),
      QObject(parent)
{
    if (!id.contains("Journal"))
        pmIdx = gPerformanceMonitor.enrol(id, "Data Recorder");
}

DataRecorder::~DataRecorder()
{
    stopRecording();
}

void DataRecorder::startRecording()
{
    QString fileName = id + "Data.csv";
    dataFile = new QFile( fileName );
    if ( !dataFile->open(QIODevice::WriteOnly) ) {
        qWarning() << "Recording failure." << QString("Could not open %1").arg(fileName);
        delete dataFile;
        dataFile = NULL;
    }
    dataStream = new QTextStream(dataFile);
    *dataStream << header << gDataNewline;
}

void DataRecorder::startRecording(double fps)
{
    startRecording();
    videoWriter = new VideoWriter();
    firstFrame = true;
    videoIdx = 1;
    this->fps = fps;
}

void DataRecorder::stopRecording()
{
    if (videoWriter)
        videoWriter->release();

    if (dataFile){
        dataFile->close();
        dataFile->deleteLater();
    }
    delete dataStream;
    delete videoWriter;

    dataStream = NULL;
    dataFile = NULL;
    videoWriter = NULL;
}

void DataRecorder::newData(EyeData eyeData)
{
    storeData(eyeData);
}

void DataRecorder::newData(FieldData fieldData)
{
    storeData(fieldData);
}

void DataRecorder::newData(DataTuple dataTuple)
{
	// Note that the Journal data recorder isn't registered with the
    // performance monitor since it's cheap to store its data.

    if (dataStream == NULL)
        return;

    if (dataStream->status() == QTextStream::Ok) {
        *dataStream << dataTuple.toQString();
        *dataStream << gDataNewline;
    }
}

bool DataRecorder::splitVideoFile()
{
    currentVideoFileInfo.refresh();
    if (currentVideoFileInfo.size() >= 1001390080 ) // to deal with opencv's avi size limitation (i.e., 2GB)
        return true;
    else
        return false;
}

template <class T>
void DataRecorder::storeData(T &data)
{
    if (firstFrame) {
        firstFrame = false;

        // TODO: Make the container and codecs parametrizable
        // Alternative containers are .mkv and .mov, for which ffmpeg seems to use
        // fourcc codes

        /* OpenCV seems capable of using other containers now :D
         *
         * ffmpeg seems to use its own codes for tags instead of regular fourcc codes
         * when mp4 container is selected; for most fourcc codes end up being invalid
         * and it falls back to a default tag.
         *
         * From the ffmpeg source (libavformat/isom.c)
         *
         *    { AV_CODEC_ID_MPEG4       , 0x20 },
         *    { AV_CODEC_ID_H264        , 0x21 },
         *    { AV_CODEC_ID_MJPEG       , 0x6C }, // 10918-1
         *
         *
         * 	MPEG4 and H264 are compressed.
         *  MJPEG is uncompressed (and therefore gives huge files!)
         *
         */
        QString fileName = id + ".mp4";
		int codec = 0x6C;

        if (videoWriter->isOpened())
            videoWriter->release();

        if (!videoWriter->open(fileName.toStdString(), codec, this->fps, Size(data.input.cols, data.input.rows), data.input.channels() == 1 ? false : true))
            qWarning() << "Recording failure." << QString("Could not open %1").arg(fileName);

        currentVideoFileInfo.setFile(fileName);

        videoIdx++;
    }

	if ( gPerformanceMonitor.shouldDrop(pmIdx, gTimer.elapsed() - data.timestamp, 2000) )
        return;

    if (videoWriter->isOpened())
        *videoWriter << data.input;
    if (dataStream->status() == QTextStream::Ok)
		*dataStream << data.toQString() << gDataNewline;

	// TODO: add recording timestamp?
}
