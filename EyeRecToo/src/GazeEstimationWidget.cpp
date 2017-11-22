#include "GazeEstimationWidget.h"
#include "ui_GazeEstimationWidget.h"

using namespace std;
using namespace cv;

static int gCollectionTupleId = qRegisterMetaType<CollectionTuple>("CollectionTuple");
static int gVectorCollectionTupleId = qRegisterMetaType<std::vector<CollectionTuple> >("std::vector<CollectionTuple>");
static int gCollectionTupleTypeId = qRegisterMetaType<CollectionTuple::TupleType>("CollectionTuple::TupleType");

GazeEstimationWidget::GazeEstimationWidget(QWidget *parent) :
	ERWidget(parent),
    isCollecting(false),
    isSampling(false),
    lastStatus(false),
    calibrationRequested(false),
    ui(new Ui::GazeEstimationWidget)
{
    ui->setupUi(this);

    setWindowTitle("Gaze Estimation Widget");
    setWindowIcon(QIcon(":/icons/gazeEstimationWidget.png"));

    gazeEstimationThread = new QThread();
    gazeEstimationThread->setObjectName("Gaze Estimation");
    gazeEstimationThread->start();
    gazeEstimationThread->setPriority(QThread::NormalPriority);

    gazeEstimation = new GazeEstimation();
    gazeEstimation->moveToThread(gazeEstimationThread);

    connect(this, SIGNAL(newTuple(CollectionTuple)),
            gazeEstimation, SLOT(addTuple(CollectionTuple)) );
    connect(this, SIGNAL(newTuples(std::vector<CollectionTuple>)),
            gazeEstimation, SLOT(addTuples(std::vector<CollectionTuple>)) );
    connect(this, SIGNAL(saveTuplesToFile(CollectionTuple::TupleType,QString)),
            gazeEstimation, SLOT(saveTuplesToFile(CollectionTuple::TupleType,QString)) );
    connect(this, SIGNAL(loadTuplesFromFile(CollectionTuple::TupleType,QString)),
            gazeEstimation, SLOT(loadTuplesFromFile(CollectionTuple::TupleType,QString)) );

    connect(this, SIGNAL(calibrationRequest()),
            gazeEstimation, SLOT(calibrate()) );
    connect(this, SIGNAL(resetCalibration(CollectionTuple::TupleType)),
            gazeEstimation, SLOT(reset(CollectionTuple::TupleType)) );
    connect(this, SIGNAL(setCalibrating(bool)),
            gazeEstimation, SLOT(setCalibrating(bool)) );

    connect(this, SIGNAL(inDataTuple(DataTuple)),
            gazeEstimation, SLOT(estimate(DataTuple)) );
    connect(gazeEstimation, SIGNAL(gazeEstimationDone(DataTuple)),
            this, SIGNAL(outDataTuple(DataTuple)) );

    connect(gazeEstimation, SIGNAL(calibrationFinished(bool,QString)),
            this, SLOT(updateStatus(bool,QString)) );

    // TODO: right now options from widget and gaze estimation are mixed in a single file, separate them
    settings = new QSettings(gCfgDir + "/" + "Gaze Estimation", QSettings::IniFormat);
    cfg.load(settings);

    // QSignalBlocker blocker(this) does not block the GUI elements
    // so we block everything manually

    ui->collectionTypeComboBox->addItem("Calibration", CollectionTuple::CALIBRATION);
    ui->collectionTypeComboBox->addItem("Evaluation", CollectionTuple::EVALUATION);

    ui->inputTypeBox->blockSignals(true);
    ui->inputTypeBox->addItem("Binocular (Mean POR)", GazeEstimationMethod::BINOCULAR_MEAN_POR);
    ui->inputTypeBox->addItem("Binocular (Mean Pupil)", GazeEstimationMethod::BINOCULAR);
    ui->inputTypeBox->addItem("Mono Left", GazeEstimationMethod::MONO_LEFT);
    ui->inputTypeBox->addItem("Mono Right", GazeEstimationMethod::MONO_RIGHT);
    for (int i=0; i<ui->inputTypeBox->count(); i++)
        if (ui->inputTypeBox->itemData(i) == cfg.inputType)
            ui->inputTypeBox->setCurrentIndex(i);
    ui->inputTypeBox->blockSignals(false);

    ui->methodBox->blockSignals(true);
    for (int i=0; i<gazeEstimation->availableGazeEstimationMethods.size(); i++) {
        QString name = gazeEstimation->availableGazeEstimationMethods[i]->description().c_str();
        ui->methodBox->addItem(name, name);
        if (name == cfg.gazeEstimationMethod)
            ui->methodBox->setCurrentIndex(i);
    }
    ui->methodBox->blockSignals(false);

    ui->samplingTimeMsCheckBox->blockSignals(true);
    ui->samplingTimeMsCheckBox->setValue(cfg.samplingTimeMs);
    ui->samplingTimeMsCheckBox->blockSignals(false);

    ui->samplingMedianCheckBox->blockSignals(true);
    ui->samplingMedianCheckBox->setChecked(cfg.useMedian);
    ui->samplingMedianCheckBox->blockSignals(false);

    ui->outlierRemovalBox->blockSignals(true);
    ui->outlierRemovalBox->setChecked(cfg.removeOutliers);
    ui->outlierRemovalBox->blockSignals(false);

    ui->outlierPupilRatioBox->blockSignals(true);
    ui->outlierPupilRatioBox->setChecked(cfg.pupilRatioOutliers);
    ui->outlierPupilRatioBox->blockSignals(false);

    ui->outlierPupilPositionBox->blockSignals(true);
    ui->outlierPupilPositionBox->setChecked(cfg.pupilPositionOutliers);
    ui->outlierPupilPositionBox->blockSignals(false);

    ui->outlierPupilOutlineBox->blockSignals(true);
    ui->outlierPupilOutlineBox->setChecked(cfg.pupilOutlineOutliers);
    ui->outlierPupilOutlineBox->blockSignals(false);

    ui->autoEvaluationBox->blockSignals(true);
    ui->autoEvaluationBox->setChecked(cfg.autoEvaluation);
    ui->autoEvaluationBox->blockSignals(false);

    ui->horizontalStrideSB->blockSignals(true);
    ui->horizontalStrideSB->setValue(100*cfg.horizontalStride);
    ui->horizontalStrideSB->blockSignals(false);

    ui->verticalStrideSB->blockSignals(true);
    ui->verticalStrideSB->setValue(100*cfg.verticalStride);
    ui->verticalStrideSB->blockSignals(false);

    ui->rangeFactorSB->blockSignals(true);
    ui->rangeFactorSB->setValue(100*cfg.rangeFactor);
    ui->rangeFactorSB->blockSignals(false);

    ui->granularitySB->blockSignals(true);
    ui->granularitySB->setValue(cfg.granularity);
    ui->granularitySB->blockSignals(false);

    ui->visualizationGroupBox->blockSignals(true);
    ui->visualizationGroupBox->setChecked(cfg.visualize);
    ui->visualizationGroupBox->blockSignals(false);

    ui->visualizationTimeSpinBox->blockSignals(true);
    ui->visualizationTimeSpinBox->setValue(cfg.visualizationTimeS);
    ui->visualizationTimeSpinBox->blockSignals(false);

    statusBarLabel = new QLabel();
    ui->statusBar->addPermanentWidget(statusBarLabel, 1000);

    gazeEstimation->settings = settings;
    QMetaObject::invokeMethod(gazeEstimation, "updateConfig");

    // Load sound effects
    loadSoundEffect(startSound, "cal-start.wav");
    loadSoundEffect(successSound, "cal-success.wav");
    loadSoundEffect(failureSound, "cal-failure.wav");
    loadSoundEffect(collectedSound, "cal-collected.wav");
}

GazeEstimationWidget::~GazeEstimationWidget()
{
    delete ui;

    if (gazeEstimation)
        gazeEstimation->deleteLater();

    if (settings)
        settings->deleteLater();

    gazeEstimationThread->quit();
    gazeEstimationThread->wait();
}

/*
 * Sampling from mouse selection
*/
void GazeEstimationWidget::startSampling(Timestamp timestamp, QPoint calibrationPoint, QSize previewSize)
{
    Q_UNUSED(timestamp)
    if (isSampling)
        return;
    isSampling = true;
    fieldRatio = cv::Point2f(
                     calibrationPoint.x() / double(previewSize.width()),
                     calibrationPoint.y() / double(previewSize.height())
                    );
    connect(this, SIGNAL(inDataTuple(DataTuple)), this, SLOT(newSample(DataTuple)) );
    QTimer::singleShot(cfg.samplingTimeMs, this, SLOT(finishSampling()));
}
template <typename T>
double median(T begin, T end) {
    T mid = begin + (end - begin) / 2;

    std::nth_element(begin, mid, end);

    if ((end - begin) % 2 != 0) {
        return *mid;
    } else {
        T lmid = std::max_element(begin, mid);
        return (*mid + *lmid) / 2.0;
    }
}
void GazeEstimationWidget::finishSampling()
{
    isSampling = false;
    disconnect(this, SIGNAL(inDataTuple(DataTuple)),
               this, SLOT(newSample(DataTuple)) );

    if (samples.size() == 0)
        return;

    if (cfg.useMedian) {
        std::vector<double> lpx, lpy, rpx, rpy;
        for (uint i=0; i<samples.size(); i++) {
            lpx.push_back(samples[i].lEye.pupil.center.x);
            lpy.push_back(samples[i].lEye.pupil.center.y);
            rpx.push_back(samples[i].rEye.pupil.center.x);
            rpy.push_back(samples[i].rEye.pupil.center.y);
        }
        CollectionTuple medianTuple = samples[0];
        medianTuple.lEye.pupil.center.x = median(lpx.begin(), lpx.end());
        medianTuple.lEye.pupil.center.y = median(lpy.begin(), lpy.end());
        medianTuple.rEye.pupil.center.x = median(rpx.begin(), rpx.end());
        medianTuple.rEye.pupil.center.y = median(rpy.begin(), rpy.end());
        medianTuple.tupleType = currentTupleType;
        emit newTuple( medianTuple );
    } else {
        for (uint i=0; i<samples.size(); i++)
            samples[i].tupleType = currentTupleType;
        emit newTuples( samples );
    }

    samples.clear();
    collectedSound.play();
}
void GazeEstimationWidget::newSample(DataTuple dataTuple)
{
    dataTuple.field.collectionMarker.center =  cv::Point3f(
                        dataTuple.field.input.cols*fieldRatio.x,
                        dataTuple.field.input.rows*fieldRatio.y,
                        0 );
    samples.push_back(dataTuple);
}

/*
 * File loading / saving
 */

void GazeEstimationWidget::on_saveTuples_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    "Save Tuples", "",
                                                    "EyeRec Tuples (*.tup);;All Files (*)");
    emit saveTuplesToFile(currentTupleType, fileName);
}

void GazeEstimationWidget::on_loadTuples_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    "Load Tuples", "",
                                                    "EyeRec Tuples (*.tup);;All Files (*)");
    emit loadTuplesFromFile(currentTupleType, fileName);
}

void GazeEstimationWidget::on_startFinishButton_toggled(bool checked)
{
    if (checked) {
        ui->startFinishButton->setText("Finish");
        statusBarLabel->setText("Calibrating: click on the field widget\nor hold 'c' to collect from marker.");
        statusBarLabel->setStyleSheet("QLabel { color : black; font : bold }");
        ui->loadTuples->setEnabled(false);
        ui->saveTuples->setEnabled(false);
        emit resetCalibration( currentTupleType );
        emit setCalibrating(true);
        connect(this, SIGNAL(newClick(Timestamp,QPoint,QSize)),
                this, SLOT(startSampling(Timestamp,QPoint,QSize)) );
        startSound.play();
        isCollecting = true;
    } else {
        ui->startFinishButton->setText("Start");
        ui->loadTuples->setEnabled(true);
        ui->saveTuples->setEnabled(true);
        disconnect(this, SIGNAL(newClick(Timestamp,QPoint,QSize)),
                   this, SLOT(startSampling(Timestamp,QPoint,QSize)) );
        emit setCalibrating(false);
        calibrationRequested = true;
        emit calibrationRequest();
        isCollecting = false;
    }
}

void GazeEstimationWidget::on_collectionTypeComboBox_currentIndexChanged(int index)
{
    currentTupleType = static_cast<CollectionTuple::TupleType>( ui->collectionTypeComboBox->itemData(index).toInt() );
}

/*
void GazeEstimationWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat())
        return;
    switch (event->key()) {
        case Qt::Key_C:
            if (isCollecting) {
                connect(this, SIGNAL(inDataTuple(DataTuple)), this, SLOT(collectMarkerTuple(DataTuple)) );
                collectedSound.play();
            }
            break;
        case Qt::Key_S:
            ui->startFinishButton->setChecked(!ui->startFinishButton->isChecked());
            break;
        default:
            break;
    }
}
void GazeEstimationWidget::keyReleaseEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat())
        return;
    switch (event->key()) {
        case Qt::Key_C:
            disconnect(this, SIGNAL(inDataTuple(DataTuple)), this, SLOT(collectMarkerTuple(DataTuple)) );
            if (isCollecting)
                collectedSound.play();
            break;
        default:
            break;
    }
}
*/

void GazeEstimationWidget::toggleCalibration()
{
	ui->startFinishButton->click();
	//ui->startFinishButton->setChecked(!ui->startFinishButton->isChecked());
}

void GazeEstimationWidget::enableMarkerCollection()
{
	if (isCollecting) {
		connect(this, SIGNAL(inDataTuple(DataTuple)), this, SLOT(collectMarkerTuple(DataTuple)) );
		collectedSound.play();
	}
}

void GazeEstimationWidget::disableMarkerCollection()
{
	disconnect(this, SIGNAL(inDataTuple(DataTuple)), this, SLOT(collectMarkerTuple(DataTuple)) );
	if (isCollecting)
		collectedSound.play();
}

void GazeEstimationWidget::collectMarkerTuple(DataTuple dataTuple)
{
    if (dataTuple.field.collectionMarker.id == -1)
        return;

    CollectionTuple collectionTuple = dataTuple;
    collectionTuple.tupleType = currentTupleType;
    emit newTuple( collectionTuple );
}

void GazeEstimationWidget::updateConfig()
{
    cfg.save(settings);
    QMetaObject::invokeMethod(gazeEstimation, "updateConfig");
}

void GazeEstimationWidget::on_samplingTimeMsCheckBox_editingFinished()
{
    cfg.samplingTimeMs = ui->samplingTimeMsCheckBox->value();
    updateConfig();
}

void GazeEstimationWidget::on_samplingMedianCheckBox_toggled(bool checked)
{
    cfg.useMedian = checked;
    updateConfig();
}

void GazeEstimationWidget::on_methodBox_currentIndexChanged(const QString &arg1)
{
    cfg.gazeEstimationMethod = arg1;
    updateConfig();
}

void GazeEstimationWidget::on_inputTypeBox_currentIndexChanged(int index)
{
    cfg.inputType = (GazeEstimationMethod::InputType) ui->inputTypeBox->itemData(index).toInt();
    updateConfig();
}

void GazeEstimationWidget::on_outlierRemovalBox_toggled(bool arg1)
{
    cfg.removeOutliers = arg1;
    updateConfig();
}

void GazeEstimationWidget::on_outlierPupilRatioBox_toggled(bool checked)
{
   cfg.pupilRatioOutliers = checked;
   updateConfig();
}

void GazeEstimationWidget::on_outlierPupilPositionBox_toggled(bool checked)
{
   cfg.pupilPositionOutliers = checked;
   updateConfig();
}

void GazeEstimationWidget::on_outlierPupilOutlineBox_toggled(bool checked)
{
   cfg.pupilOutlineOutliers = checked;
   updateConfig();
}

void GazeEstimationWidget::on_autoEvaluationBox_toggled(bool checked)
{
    cfg.autoEvaluation = checked;
    updateConfig();
}

void GazeEstimationWidget::on_horizontalStrideSB_valueChanged(int arg1)
{
    cfg.horizontalStride = arg1 / 100.0;
    updateConfig();
}

void GazeEstimationWidget::on_verticalStrideSB_valueChanged(int arg1)
{
    cfg.verticalStride = arg1 / 100.0;
    updateConfig();
}

void GazeEstimationWidget::on_granularitySB_valueChanged(int arg1)
{
    cfg.granularity = arg1;
    updateConfig();
}

void GazeEstimationWidget::on_rangeFactorSB_valueChanged(int arg1)
{
    cfg.rangeFactor = arg1 / 100.0;
    updateConfig();
}

void GazeEstimationWidget::on_visualizationGroupBox_toggled(bool arg1)
{
    cfg.visualize = arg1;
    updateConfig();
}

void GazeEstimationWidget::on_visualizationTimeSpinBox_valueChanged(int arg1)
{
    cfg.visualizationTimeS = arg1;
    updateConfig();
}

void GazeEstimationWidget::updateStatus(bool status, QString msg)
{
    // There are two different triggers for an status update:
    // 1) We requested a calibration, in which case we always give the user feedback
    // 2) The configuration changed, in which case we only give feedback if the status changed
    if (calibrationRequested || status != lastStatus)
        status ? successSound.play() : failureSound.play();
    lastStatus = status;
    calibrationRequested = false;

    if (status) {
        statusBarLabel->setText("Calibrated.");
        statusBarLabel->setStyleSheet("QLabel { font : bold; color : green }");
    } else {
        statusBarLabel->setText(QString("Uncalibrated: %1").arg(msg));
        statusBarLabel->setStyleSheet("QLabel { font : bold; color : red }");
    }

}

