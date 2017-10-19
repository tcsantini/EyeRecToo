#include "CameraWidget.h"
#include "ui_CameraWidget.h"

using namespace std;
using namespace cv;

CameraWidget::CameraWidget(QString id, ImageProcessor::Type type, QWidget *parent) :
    QMainWindow(parent),
    id(id),
    type(type),
    lastTimestamp(0),
    sROI(QPoint(0,0)),
    eROI(QPoint(0,0)),
    settingROI(false),
    lastUpdate(0),
    updateIntervalMs(50),
	maxAgeMs(300),
	cameraCalibrationSampleRequested(false),
    ui(new Ui::CameraWidget)
{
    ui->setupUi(this);
    ui->viewFinder->setScaledContents(false);
    ui->viewFinder->setAttribute(Qt::WA_OpaquePaintEvent, true);
    ui->viewFinder->setCursor(Qt::CrossCursor);

    switch (type) {
        case ImageProcessor::Eye:
            ui->viewFinder->setToolTip("You can select a region of interest by clicking and holding the right mouse button.");
            break;
        case ImageProcessor::Field:
            ui->viewFinder->setToolTip("After starting the collection you can select points by clicking in this view with the left mouse button.");
            break;
        default:
            break;
    }

    // Early enrollment of stages so they are listed in order :-)
    gPerformanceMonitor.enrol(id, "Frame Grabber");
    gPerformanceMonitor.enrol(id, "Image Processor");
    gPerformanceMonitor.enrol(id, "Data Recorder");

    setWindowTitle(id + " Widget");
    camera = NULL;
    imageProcessor = NULL;

    // Camera
    cameraThread = new QThread();
    cameraThread->setObjectName(id + " Camera");
    cameraThread->start();
    cameraThread->setPriority(QThread::TimeCriticalPriority);
    camera = new Camera(id);
    camera->moveToThread(cameraThread);
    connect(camera, SIGNAL(noCamera(QString)),
            this, SLOT(noCamera(QString)));
    // debugging preview
    //connect(camera, SIGNAL(newFrame(Timestamp, const cv::Mat&)),
    //    this, SLOT(preview(Timestamp, const cv::Mat&)) );

    // Image Processor
    processorThread = new QThread();
    processorThread->setObjectName(id + " Processor");
    processorThread->start();
    processorThread->setPriority(QThread::TimeCriticalPriority);
    imageProcessor = new ImageProcessor(id, type);
    imageProcessor->moveToThread(processorThread);
    switch (type) {
        case ImageProcessor::Eye:
            imageProcessor->eyeProcessorUI = new EyeImageProcessorUI;
            connect(imageProcessor, SIGNAL(newData(EyeData)),
                    this, SLOT(preview(EyeData)) );
            connect(imageProcessor, SIGNAL(newData(EyeData)),
                    this, SIGNAL(newData(EyeData)) );
            break;
        case ImageProcessor::Field:
            imageProcessor->fieldProcessorUI = new FieldImageProcessorUI;
            connect(imageProcessor, SIGNAL(newData(FieldData)),
                this, SLOT(preview(FieldData)) );
            connect(imageProcessor, SIGNAL(newData(FieldData)),
                this, SIGNAL(newData(FieldData)) );
            break;
    }
    QMetaObject::invokeMethod(imageProcessor, "create");
    connect(camera, SIGNAL(newFrame(Timestamp, const cv::Mat&)),
        imageProcessor, SIGNAL(process(Timestamp, const cv::Mat&)) );

    // Data Recorder
    recorderThread = new QThread();
    recorderThread->setObjectName(id + " Recorder");
    recorderThread->start();
    recorderThread->setPriority(QThread::NormalPriority);
    recorder = new DataRecorderThread(id, type == ImageProcessor::Eye ? EyeData().header() : FieldData().header());
    recorder->moveToThread(recorderThread);
	QMetaObject::invokeMethod(recorder, "create");

	cameraCalibration = new CameraCalibration();
	connect(cameraCalibration, SIGNAL(requestSample()),
			this, SLOT(requestCameraCalibrationSample()) );
	connect(cameraCalibration, SIGNAL(calibrationFinished(bool)),
			this, SLOT(onCameraCalibrationFinished(bool)) );
	cameraCalibration->load(gCfgDir + "/" + id + "Calibration.xml");

    // GUI
    optionsGroup = new QActionGroup(this);
    connect(optionsGroup, SIGNAL(triggered(QAction*)),
            this, SLOT(options(QAction*)));

    optionAction = new QAction("Camera", optionsGroup);
    optionAction->setData(QVariant::fromValue(optionAction->text()));
    optionAction->setCheckable(false);
    ui->menuOptions->addAction(optionAction);

    optionAction = new QAction("Image Processor", optionsGroup);
    optionAction->setData(QVariant::fromValue(optionAction->text()));
    optionAction->setCheckable(false);
    ui->menuOptions->addAction(optionAction);

    optionAction = new QAction("Calibrate Camera", optionsGroup);
    optionAction->setData(QVariant::fromValue(optionAction->text()));
    optionAction->setCheckable(false);
    ui->menuOptions->addAction(optionAction);

    connect(this, SIGNAL(newROI(QPointF,QPointF)),
            imageProcessor, SIGNAL(newROI(QPointF,QPointF)) );

    font.setStyleHint(QFont::Monospace);
    QMetaObject::invokeMethod(camera, "loadCfg");
}

CameraWidget::~CameraWidget()
{
    delete ui;

    if (camera) {
        camera->deleteLater();
        camera = NULL;
    }

    if (imageProcessor) {
        imageProcessor->deleteLater();
        imageProcessor = NULL;
    }

    if (recorder) {
        recorder->deleteLater();
        recorder = NULL;
	}

	if (cameraCalibration) {
		cameraCalibration->deleteLater();
		cameraCalibration = NULL;
	}

    cameraThread->quit();
    cameraThread->wait();

    processorThread->quit();
    processorThread->wait();

    recorderThread->quit();
	recorderThread->wait();
}

void CameraWidget::preview(Timestamp t, const cv::Mat &frame)
{
    updateFrameRate(t);
    if (!shouldUpdate(t))
        return;
    if (!isDataRecent(t))
        return;
    QImage scaled = previewImage(frame);

    QPainter painter(&scaled);
    drawROI(painter);
    ui->viewFinder->setPixmap(QPixmap::fromImage(scaled));
}

void CameraWidget::preview(EyeData data)
{
    if (data.input.empty()) // if there's no eye image, don't update
        return;

    updateFrameRate(data.timestamp);
    if (!shouldUpdate(data.timestamp))
        return;
    if (!isDataRecent(data.timestamp))
        return;

    QImage scaled = previewImage(data.input);

    QPainter painter(&scaled);
    drawROI(painter);
    if (data.validPupil)
        drawPupil(data.pupil, painter);
	ui->viewFinder->setPixmap(QPixmap::fromImage(scaled));

	sendCameraCalibrationSample(data.input);
}

void CameraWidget::preview(FieldData data)
{
    // We only update the camera frame rate here. Viewfinder is updated
    // based on the signal from the gaze estimation stage
    updateFrameRate(data.timestamp);
}
void CameraWidget::preview(const DataTuple &data)
{
    if (data.field.input.empty()) // If there's no field image, don't update
        return;

    if (!shouldUpdate(data.timestamp))
        return;
    if (!isDataRecent(data.field.timestamp))
        return;

    Mat input = data.field.input;

    if (data.showGazeEstimationVisualization) {
        if (data.gazeEstimationVisualization.empty())
            return;
        input = data.gazeEstimationVisualization;
    }

    QImage scaled = previewImage(input);

    QPainter painter(&scaled);
    for (int i=0; i<data.field.markers.size(); i++)
        drawMarker(data.field.markers[i], painter, Qt::cyan);
    if (data.field.collectionMarker.id != -1)
        drawMarker(data.field.collectionMarker, painter, Qt::green);
    if (data.field.validGazeEstimate)
        drawGaze(data.field, painter);
	ui->viewFinder->setPixmap(QPixmap::fromImage(scaled));

	sendCameraCalibrationSample(input);
}

void CameraWidget::updateFrameRate(Timestamp t)
{
    dt.push_back(t-lastTimestamp);
    lastTimestamp = t;

    if (dt.size() < 30) {
        ui->statusbar->showMessage(
            QString("%1 @ N/A FPS").arg(
                camera->currentCameraInfo.description()
                )
            );
        return;
    }
    dt.pop_front();

    int acc = 0;
    for (std::list<int>::iterator it=dt.begin(); it != dt.end(); ++it)
        acc += *it;
    double fps = 1000 / (double(acc)/dt.size());

    ui->statusbar->showMessage(
        QString("%1 @ %2 FPS").arg(
            camera->currentCameraInfo.description()).arg(
            fps, 0, 'f', 2)
        );
}

void CameraWidget::noCamera(QString msg)
{
    ui->statusbar->showMessage(msg, 0);
    QImage blank(ui->viewFinder->size(), QImage::Format_RGB888);
    blank.fill(Qt::black);
    ui->viewFinder->setPixmap(QPixmap::fromImage(blank));
}

void CameraWidget::options(QAction* action)
{
    QString option = action->data().toString().toLower();

    if (option == "camera") {
        if (camera) {
            camera->ui->move(this->pos());
            QMetaObject::invokeMethod(camera, "showOptions", Qt::QueuedConnection);
        }
    }
    if (option == "image processor")
        if (imageProcessor)
            QMetaObject::invokeMethod(imageProcessor, "showOptions", Qt::QueuedConnection, Q_ARG(QPoint, this->pos()));

	if (option == "calibrate camera")
		if (cameraCalibration)
			QMetaObject::invokeMethod(cameraCalibration, "showOptions", Qt::QueuedConnection, Q_ARG(QPoint, this->pos()));
}

void CameraWidget::startRecording()
{
    ui->menubar->setEnabled(false);
    QMetaObject::invokeMethod(recorder, "startRecording", Q_ARG(double, camera->fps));
    connect(imageProcessor, SIGNAL(newData(EyeData)),
            recorder, SIGNAL(newData(EyeData)) );
    connect(imageProcessor, SIGNAL(newData(FieldData)),
            recorder, SIGNAL(newData(FieldData)) );
}

void CameraWidget::stopRecording()
{
    QMetaObject::invokeMethod(recorder, "stopRecording");
    disconnect(imageProcessor, SIGNAL(newData(EyeData)),
            recorder, SIGNAL(newData(EyeData)) );
    disconnect(imageProcessor, SIGNAL(newData(FieldData)),
            recorder, SIGNAL(newData(FieldData)) );
    ui->menubar->setEnabled(true);
}

void CameraWidget::mousePressEvent(QMouseEvent *event)
{
    if (ui->viewFinder->underMouse()) {
        if (event->button() == Qt::LeftButton)
            emit newClick( gTimer.elapsed(), ui->viewFinder->mapFromGlobal(this->mapToGlobal(event->pos())), ui->viewFinder->size());

        if (event->button() == Qt::RightButton) {
            sROI = ui->viewFinder->mapFrom(this, event->pos());
            validatePoint(sROI);
            sROI.setX( sROI.x() / ui->viewFinder->width());
            sROI.setY( sROI.y() / ui->viewFinder->height());
            eROI = QPointF();
            settingROI = true;
        }
    }
}

void CameraWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (ui->viewFinder->underMouse()) {
        if (settingROI) {
            eROI = ui->viewFinder->mapFrom(this, event->pos());
            validatePoint(eROI);
            eROI.setX( eROI.x() / ui->viewFinder->width());
            eROI.setY( eROI.y() / ui->viewFinder->height());
        }
    }
}

void CameraWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        eROI = ui->viewFinder->mapFrom(this, event->pos());
        validatePoint(eROI);
        eROI.setX( eROI.x() / ui->viewFinder->width());
        eROI.setY( eROI.y() / ui->viewFinder->height());
        settingROI = false;
        if ( abs(sROI.x() - eROI.x()) < 0.1 || abs(sROI.y() - eROI.y()) < 0.1 ) {
            sROI = QPointF();
            eROI = QPointF();
        }
        emit newROI(sROI, eROI);
    }
}

void CameraWidget::validatePoint(QPointF &point)
{
    if (point.x() < 1)
        point.setX(1);
    if (point.y() < 1)
        point.setY(1);
    if (point.x() > ui->viewFinder->width())
        point.setX(ui->viewFinder->width()-1);
    if (point.y() > ui->viewFinder->height())
        point.setY(ui->viewFinder->height()-1);
}

bool CameraWidget::shouldUpdate(Timestamp t)
{
    if (!this->isVisible())
        return false;

    // TODO: Right now, we don't update every frame to save resources.
    // Make this parametrizable or move to faster drawing methods
    if (t - lastUpdate > updateIntervalMs) {
        lastUpdate = t;
        return true;
    }

    return false;
}

bool CameraWidget::isDataRecent(Timestamp t)
{
    Timestamp cur = gTimer.elapsed();
    if (cur - t < maxAgeMs) // TODO: make this based on estimated FPS?
        return true;
    if (cur - lastUpdate > maxAgeMs) { // not recent, but we haven't update in a while
        lastUpdate = cur;
        return true;
    }
    return false;
}

QImage CameraWidget::previewImage(const cv::Mat &frame)
{
    Mat rgb;
    Size previewSize(ui->viewFinder->width(), ui->viewFinder->height());

    switch (frame.channels()) {
        case 1:
            cvtColor(frame, rgb, CV_GRAY2RGB);
            cv::resize(rgb, rgb, previewSize, CV_INTER_LINEAR);
            break;
        case 3:
            cvtColor(frame, rgb, CV_BGR2RGB);
            cv::resize(rgb, rgb, previewSize, CV_INTER_LINEAR);
            break;
        default:
            rgb = cv::Mat::zeros(previewSize, CV_8UC3);
            break;
    }
    QImage scaled = QImage(rgb.data, rgb.cols, rgb.rows, (int) rgb.step, QImage::Format_RGB888).copy();

    rw = scaled.width() / (float) frame.cols;
    rh = scaled.height() / (float) frame.rows;
    refPx = 0.01*max<int>(frame.cols, frame.rows);
    refPx = max<double>(refPx, 1.0);

    return scaled;
}

void CameraWidget::sendCameraCalibrationSample(const cv::Mat &frame)
{
	if (!cameraCalibrationSampleRequested || !cameraCalibration)
		return;
	cameraCalibrationSampleRequested = false;
	QMetaObject::invokeMethod(cameraCalibration, "newSample", Qt::QueuedConnection, Q_ARG(cv::Mat, frame));
	emit sendCameraCalibrationSample(frame);
}

void CameraWidget::onCameraCalibrationFinished(bool success)
{
	if (!success)
		return;
	// TODO: at some point we might consider storing this with a unique camera ID instead of this generic one
	QString fileName = gCfgDir + "/" + id + "Calibration.xml";
	QMetaObject::invokeMethod(cameraCalibration, "store", Qt::QueuedConnection, Q_ARG(QString, fileName));
	QMetaObject::invokeMethod(imageProcessor,  "updateConfig", Qt::QueuedConnection);
}

void CameraWidget::drawROI(QPainter &painter)
{
    QPen pen(Qt::green, 1, Qt::DotLine);
    painter.setPen(pen);
    if (sROI.isNull() || eROI.isNull())
        return;
    painter.drawRect(
                ui->viewFinder->width()*sROI.x(),
                ui->viewFinder->height()*sROI.y(),
                ui->viewFinder->width()*(eROI.x()-sROI.x()),
                ui->viewFinder->height()*(eROI.y()-sROI.y())
                );
}

void CameraWidget::drawPupil(const cv::RotatedRect ellipse, QPainter &painter)
{

    painter.scale(rw, rh);
    painter.setPen(QPen(Qt::green, 0.75*refPx, Qt::SolidLine));
    painter.setBrush(Qt::green);
    // center
    double r = 0.5*refPx;
    painter.drawEllipse(QPointF(ellipse.center.x, ellipse.center.y), r, r);
    painter.setBrush(QBrush(Qt::NoBrush));
    // outline
    if (ellipse.size.width > 0 && ellipse.size.height > 0){
        painter.translate(ellipse.center.x, ellipse.center.y);
        painter.rotate(ellipse.angle);
        painter.drawEllipse( -0.5*ellipse.size.width, -0.5*ellipse.size.height, ellipse.size.width, ellipse.size.height );
    }
    painter.resetTransform();
}

void CameraWidget::drawMarker(const Marker &marker, QPainter &painter, QColor color)
{
    painter.setPen(QPen(color, 0.25*refPx, Qt::SolidLine));
    font.setPointSize(2*refPx);
    painter.setFont(font);

    painter.scale(rw, rh);

    QPointF contour[4];
    for (unsigned int i=0; i<4; i++)
        contour[i] = QPointF(marker.corners[i].x, marker.corners[i].y);
    painter.drawConvexPolygon(contour, 4);
    int delta = 10*refPx;
    painter.drawText(
                QRectF(marker.center.x-delta/2, marker.center.y-delta/2, delta, delta),
                Qt::AlignCenter|Qt::TextWordWrap,
                QString("%1\n(%2)").arg(marker.id).arg(marker.center.z, 0, 'g', 2)
            );
    //painter.drawText(marker.center.x, marker.center.y, QString("%1 (%2)").arg(marker.id).arg(marker.center.z, 0, 'g', 2));
    //for (int i=0; i<4; i++)
    //    painter.drawText(contour[i], QString::number(i));
    painter.drawPoint(marker.center.x, marker.center.y);
    painter.resetTransform();
}

void CameraWidget::drawGaze(const FieldData &field, QPainter &painter)
{
    painter.setPen(QPen(Qt::red, 0.5*refPx, Qt::SolidLine));
    double radius = 2.5*refPx; // TODO: change based on evaluation error :-)
    painter.scale(rw, rh);
    if (radius > 0) {
        painter.drawEllipse(field.gazeEstimate.x - radius, field.gazeEstimate.y - radius , 2*radius, 2*radius);
        painter.drawLine( field.gazeEstimate.x, 0, field.gazeEstimate.x, field.width );
        painter.drawLine( 0, field.gazeEstimate.y, field.width, field.gazeEstimate.y );
    }
    painter.resetTransform();
}

