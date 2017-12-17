#include "CameraWidget.h"
#include "ui_CameraWidget.h"

using namespace std;
using namespace cv;

CameraWidget::CameraWidget(QString id, ImageProcessor::Type type, QWidget *parent) :
	ERWidget(parent),
    id(id),
    type(type),
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

	cameraCalibration = new CameraCalibration(id);
	connect(cameraCalibration, SIGNAL(requestSample()),
			this, SLOT(requestCameraCalibrationSample()) );
	connect(cameraCalibration, SIGNAL(calibrationFinished(bool)),
			this, SLOT(onCameraCalibrationFinished(bool)) );
	cameraCalibration->load(gCfgDir + "/" + id + "Calibration.xml");

    // Image Processor
    processorThread = new QThread();
    processorThread->setObjectName(id + " Processor");
    processorThread->start();
    processorThread->setPriority(QThread::TimeCriticalPriority);
	imageProcessor = new ImageProcessor(id, type);
	imageProcessor->cameraCalibration = cameraCalibration;
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

	// Initial roi
	setROI( QPointF(0, 0), QPointF(1, 1) );
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

	updateWidgetSize(data.input.cols, data.input.rows);

	QImage scaled = previewImage(data.input);
	QRectF userROI = {
			QPointF( ui->viewFinder->width()*sROI.x(), ui->viewFinder->height()*sROI.y() ),
			QPointF( ui->viewFinder->width()*(eROI.x()), ui->viewFinder->height()*(eROI.y()) )
		};
	QRectF coarseROI = {
		QPointF(
			ui->viewFinder->width() * data.coarseROI.tl().x / (float) data.input.cols,
			ui->viewFinder->height() * data.coarseROI.tl().y / (float) data.input.rows
			),
		QPointF(
			ui->viewFinder->width() * data.coarseROI.br().x / (float) data.input.cols,
			ui->viewFinder->height() * data.coarseROI.br().y / (float) data.input.rows
			)
		};

	eyeOverlay.drawOverlay(data, userROI, coarseROI, scaled);
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

	Mat input;
	if (data.showGazeEstimationVisualization && !data.gazeEstimationVisualization.empty())
		input = data.gazeEstimationVisualization;
	else
		input = data.field.input;

	updateWidgetSize(input.cols, input.rows);

	QImage scaled = previewImage(input);
	fieldOverlay.drawOverlay(data, scaled);;
	ui->viewFinder->setPixmap(QPixmap::fromImage(scaled));

	sendCameraCalibrationSample(input);
}

void CameraWidget::updateFrameRate(Timestamp t)
{
	// Simpler version since the status bar update showed some overhead
	// during tests with OpenGL
	if (tq.size() == 0) {
		ui->statusbar->showMessage(
			QString("%1 @ N/A FPS").arg(
					camera->currentCameraInfo.description()
				)
			);
		tq.push_back(t);
		lastFrameRateUpdate = t;
		return;
	}

	tq.push_back(t);
	if (tq.size() > 30) {
		tq.pop_front();
		if (tq.back() - lastFrameRateUpdate > 100) { // update every 100 ms
			double fps = (tq.size()-1) / (1.0e-3*( tq.back() - tq.front() ) );
			lastFrameRateUpdate = tq.back();
			ui->statusbar->showMessage(
				QString("%1 @ %2 FPS").arg(
					camera->currentCameraInfo.description()).arg(
						fps, 0, 'f', 2
					)
				);
		}
	}

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
	disconnect(imageProcessor, SIGNAL(newData(EyeData)),
			recorder, SIGNAL(newData(EyeData)) );
	disconnect(imageProcessor, SIGNAL(newData(FieldData)),
			recorder, SIGNAL(newData(FieldData)) );
	QMetaObject::invokeMethod(recorder, "stopRecording", Qt::QueuedConnection);
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
			eROI = sROI;
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
		setROI(sROI, eROI);
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

	/* TODO: Right now, we don't update every frame to save resources.
	 * Make this parametrizable or move to faster drawing methods.
	 *
	 * Notes: I've tried some initial tests to move to OpenGL, which were
	 * unsuccessful. This includes using QPainter with QOpenGLWidgets, native
	 * OpenGL directives (using QUADs, TRIANGLES, and an FBO).
	 * In all cases I saw a similar behavior: scaling/drawing is faster, but I
	 * get CPU hogs from other places -- e.g., additional calls to nvogl,
	 * NtGdiDdDDIEscape (whatever that is!), and calls to wglMakeCurrent
	 * (which seem to be rather expensive!).
	 * If anyone who actually knows OpenGL wanna try, be my guest :-)
	 */
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
    Size previewSize(ui->viewFinder->width(), ui->viewFinder->height());

    switch (frame.channels()) {
        case 1:
			cvtColor(frame, rgb, CV_GRAY2RGB);
			cv::resize(rgb, resized, previewSize, cv::INTER_NEAREST);
			break;
        case 3:
			cvtColor(frame, rgb, CV_BGR2RGB);
			cv::resize(rgb, resized, previewSize, cv::INTER_NEAREST);
            break;
        default:
			resized = cv::Mat::zeros(previewSize, CV_8UC3);
            break;
	}

	/* TODO: according to http://doc.qt.io/qt-5/qpainter.html
	 *
	 * "Raster - This backend implements all rendering in pure software and is
	 * always used to render into QImages. For optimal performance only use the
	 * format types QImage::Format_ARGB32_Premultiplied, QImage::Format_RGB32 or
	 * QImage::Format_RGB16. Any other format, including QImage::Format_ARGB32,
	 * has significantly worse performance. This engine is used by default for
	 * QWidget and QPixmap."
	 *
	 * However, I've found that resizing with OpenCV using only three bytes
	 * results in faster and less cpu-intensive code.
	 * This is relative to ARGB32_Premultiplied (which should be the fastest
	 * option with QImages, according to http://doc.qt.io/qt-5/qimage.html
	 *
	 */
	return QImage(resized.data, resized.cols, resized.rows, (int) resized.step, QImage::Format_RGB888);
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
	// Do the store here with a direct call to guarantee it will be done before the updateConfig
	QMetaObject::invokeMethod(cameraCalibration, "store", Qt::DirectConnection, Q_ARG(QString, fileName));
	QMetaObject::invokeMethod(imageProcessor,  "updateConfig", Qt::QueuedConnection);
}

void CameraWidget::updateWidgetSize(const int &width, const int &height)
{
	// Logic to limit the size of the camera widgets
	QSize newFrameSize = { width, height };
	if (frameSize == newFrameSize)
		return;

	frameSize = newFrameSize;
	QSize minMaxSize = { 640 , 480 };
	QSize maxSize = { 960 , 540 };
	if (frameSize.width() < maxSize.width() && frameSize.height() < maxSize.height() ) {
		if (frameSize.width() < minMaxSize.width() && frameSize.height() < minMaxSize.height()) {
			float ratio = minMaxSize.width() / (float) frameSize.width();
			frameSize.setWidth(minMaxSize.width());
			frameSize.setHeight(ratio * minMaxSize.height());
		}
		this->setMaximumSize( frameSize );
	} else
		this->setMaximumSize( maxSize );
}
