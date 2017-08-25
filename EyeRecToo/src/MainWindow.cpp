#include "MainWindow.h"
#include "ui_MainWindow.h"

// TODO: refactor MainWindow into a config aware class that can be shared by the
// widgets

void MainWindow::createExtraMenus()
{
    ui->menuBar->addAction("References");
    ui->menuBar->addAction("About");
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    lEyeWidget(NULL),
    rEyeWidget(NULL),
    fieldWidget(NULL),
    synchronizer(NULL),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    createExtraMenus();
    connect(ui->menuBar, SIGNAL(triggered(QAction*)), this, SLOT(menuOption(QAction*)) );

    settings = new QSettings(gCfgDir + "/" + "EyeRecToo", QSettings::IniFormat);
    cfg.load(settings);

    setWindowTitle(QString("EyeRecToo v%1").arg(GIT_VERSION));
    setWindowIcon(QIcon(":/icons/EyeRecToo.png"));
    setupWidget(this, cfg.mainWindowPos, cfg.mainWindowSize, true);

    if (!cfg.workingDirectory.isEmpty())
        setWorkingDirectory(cfg.workingDirectory);
    ui->pwd->setText(QDir::currentPath());

    ui->blinker->hide();

    logWidget = new LogWidget();
    setupWidget(logWidget, cfg.logWidgetPos, cfg.logWidgetSize, cfg.logWidgetVisible, ui->log);
    gLogWidget = logWidget;

    /*
     * WARNING: DO NOT REMOVE THIS CALL to QCameraInfo::availableCameras()
     * Logically, its meaningless, but it guarantees that DirectShow will work
     * properly by CoInitializing it in the main thread.
     */
    volatile QList<QCameraInfo> tmp = QCameraInfo::availableCameras();
    Q_UNUSED(tmp);

    /*
     * Asynchronous elements
     */
    lEyeWidget = new CameraWidget("LeftEye", ImageProcessor::Eye);
    lEyeWidget->setWindowIcon(QIcon(":/icons/lEyeWidget.png"));
    setupWidget(lEyeWidget, cfg.leftEyeWidgetPos, cfg.leftEyeWidgetSize, cfg.leftEyeWidgetVisible, ui->leftEyeCam);
    QThread::msleep(200);
    rEyeWidget = new CameraWidget("RightEye", ImageProcessor::Eye);
    rEyeWidget->setWindowIcon(QIcon(":/icons/rEyeWidget.png"));
    setupWidget(rEyeWidget, cfg.rightEyeWidgetPos, cfg.rightEyeWidgetSize, cfg.rightEyeWidgetVisible, ui->rightEyeCam);
    QThread::msleep(200);
    fieldWidget = new CameraWidget("Field", ImageProcessor::Field);
    fieldWidget->setWindowIcon(QIcon(":/icons/fieldWidget.png"));
    setupWidget(fieldWidget, cfg.fieldWidgetPos, cfg.fieldWidgetSize, cfg.fieldWidgetVisible, ui->fieldCam);

    /*
     * Synchronizer
     */
    synchronizer = new Synchronizer();
    connect(lEyeWidget,  SIGNAL(newData(EyeData)),
            synchronizer, SLOT(newLeftEyeData(EyeData)), Qt::QueuedConnection);
    connect(rEyeWidget,  SIGNAL(newData(EyeData)),
            synchronizer, SLOT(newRightEyeData(EyeData)) );
    connect(fieldWidget, SIGNAL(newData(FieldData)),
            synchronizer, SLOT(newFieldData(FieldData)) );

    /*
     * Synchronous elements
     */
    gazeEstimationWidget = new GazeEstimationWidget();
    setupWidget(gazeEstimationWidget, cfg.gazeEstimationWidgetPos, cfg.gazeEstimationWidgetSize, cfg.gazeEstimationWidgetVisible, ui->gazeEstimation);
    connect(synchronizer, SIGNAL(newData(DataTuple)),
            gazeEstimationWidget, SIGNAL(inDataTuple(DataTuple)) );
    connect(fieldWidget, SIGNAL(newClick(Timestamp,QPoint,QSize)),
            gazeEstimationWidget, SIGNAL(newClick(Timestamp,QPoint,QSize)) );

    connect(gazeEstimationWidget, SIGNAL(outDataTuple(DataTuple)),
            fieldWidget, SLOT(preview(DataTuple)) );

    journalThread = new QThread();
    journalThread->setObjectName("Journal");
    journalThread->start();
    journalThread->setPriority(QThread::NormalPriority);
    journal = new DataRecorderThread("Journal", DataTuple().header());
    journal->moveToThread(journalThread);
    QMetaObject::invokeMethod(journal, "create");

    networkStream = new NetworkStream();

    networkStream->start(2002);
    connect(gazeEstimationWidget, SIGNAL(outDataTuple(DataTuple)), networkStream, SLOT(push(DataTuple)) );

    performanceMonitorWidget = new PerformanceMonitorWidget();
    setupWidget(performanceMonitorWidget, cfg.performanceMonitorWidgetPos, cfg.performanceMonitorWidgetSize, cfg.performanceMonitorWidgetVisible, ui->performanceMonitor);

    // GUI to Widgets signals
    connect(this, SIGNAL(startRecording()),
            lEyeWidget, SLOT(startRecording()) );
    connect(this, SIGNAL(stopRecording()),
            lEyeWidget, SLOT(stopRecording()) );
    connect(this, SIGNAL(startRecording()),
            rEyeWidget, SLOT(startRecording()) );
    connect(this, SIGNAL(stopRecording()),
            rEyeWidget, SLOT(stopRecording()) );
    connect(this, SIGNAL(startRecording()),
            fieldWidget, SLOT(startRecording()) );
    connect(this, SIGNAL(stopRecording()),
            fieldWidget, SLOT(stopRecording()) );
//    connect(this, SIGNAL(startRecording()), calibrationWidget, SLOT(startRecording()) );
//    connect(this, SIGNAL(stopRecording()), calibrationWidget, SLOT(stopRecording()) );
    connect(this, SIGNAL(startRecording()),
            journal, SIGNAL(startRecording()) );
    connect(this, SIGNAL(stopRecording()),
            journal, SIGNAL(stopRecording()) );

    loadSoundEffect(recStartSound, "rec-start.wav");
    loadSoundEffect(recStopSound, "rec-stop.wav");
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::closeEvent(QCloseEvent *event)
{
    if (ui->recordingToggle->isChecked()) {
        ui->recordingToggle->setChecked(false);
        on_recordingToggle_clicked();
        // should be equivalent, but just in case click() is connected with a
        // queued connection in the future :-)
        // ui->recordingToggle->click();
    }

    cfg.mainWindowPos = pos();
    cfg.mainWindowSize = size();
    cfg.logWidgetPos = logWidget->pos();
    cfg.logWidgetSize = logWidget->size();
    cfg.logWidgetVisible = logWidget->isVisible();
    cfg.leftEyeWidgetPos = lEyeWidget->pos();
    cfg.leftEyeWidgetSize = lEyeWidget->size();
    cfg.leftEyeWidgetVisible = lEyeWidget->isVisible();
    cfg.rightEyeWidgetPos = rEyeWidget->pos();
    cfg.rightEyeWidgetSize = rEyeWidget->size();
    cfg.rightEyeWidgetVisible = rEyeWidget->isVisible();
    cfg.fieldWidgetPos = fieldWidget->pos();
    cfg.fieldWidgetSize = fieldWidget->size();
    cfg.fieldWidgetVisible = fieldWidget->isVisible();
    cfg.gazeEstimationWidgetPos = gazeEstimationWidget->pos();
    cfg.gazeEstimationWidgetSize = gazeEstimationWidget->size();
    cfg.gazeEstimationWidgetVisible = gazeEstimationWidget->isVisible();
    cfg.performanceMonitorWidgetPos = performanceMonitorWidget->pos();
    cfg.performanceMonitorWidgetSize = performanceMonitorWidget->size();
    cfg.performanceMonitorWidgetVisible = performanceMonitorWidget->isVisible();
    cfg.workingDirectory = QDir::currentPath();
    if (settings)
        cfg.save(settings);

    qInfo() << "Closing Left Eye Widget...";
    if ( lEyeWidget ) {
        lEyeWidget->close();
        lEyeWidget->deleteLater();
        lEyeWidget = NULL;
    }
    qInfo() << "Closing Right Eye Widget...";
    if ( rEyeWidget ) {
        rEyeWidget->close();
        rEyeWidget->deleteLater();
        rEyeWidget = NULL;
    }
    qInfo() << "Closing Field Widget...";
    if ( fieldWidget ) {
        fieldWidget->close();
        fieldWidget->deleteLater();
        fieldWidget = NULL;
    }

    qInfo() << "Closing Gaze Estimation Widget...";
    if (gazeEstimationWidget) {
        gazeEstimationWidget->close();
        gazeEstimationWidget->deleteLater();
    }

    qInfo() << "Stoping network stream...";
    if (networkStream)
        networkStream->deleteLater();

    gPerformanceMonitor.report();

    qInfo() << "Closing Performance Monitor Widget...";
    if (performanceMonitorWidget) {
        performanceMonitorWidget->close();
        performanceMonitorWidget->deleteLater();
        performanceMonitorWidget = NULL;
    }


    qInfo() << "Closing Log Widget...";
    if (logWidget) {
        gLogWidget = NULL;
        logWidget->close();
        logWidget->deleteLater();
        logWidget = NULL;
    }

    qInfo() << "Stopping journal and synchronizer";
    if (journal) {
        journal->deleteLater();
        journal = NULL;
    }

    if (synchronizer) {
        synchronizer->deleteLater();
        synchronizer = NULL;
    }

    if (settings) {
        settings->deleteLater();
        settings = NULL;
    }

    event->accept();
}

void MainWindow::setWorkingDirectory(QString dir)
{
    previousPwd = QDir::currentPath();
    QDir::setCurrent(dir);
    ui->pwd->setText(dir);
    qInfo() << "PWD set to" << dir;
}

void MainWindow::on_changePwdButton_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                 QDir::currentPath(),
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);

    if (!dir.isEmpty()) {
        setWorkingDirectory(dir);
    }

}

void MainWindow::setSubjectName(QString newSubjectName)
{
    QRegExp re("[a-zA-Z0-9-_]*");
    Q_ASSERT(re.isValid());

    if (!re.exactMatch(newSubjectName)) {
        QMessageBox::warning(NULL,
             "Invalid subject name.",
             QString("Invalid name: \"%1\".\n\nSubject names may contain only letters, numbers, dashes (-), and underscores (_).\nIn regex terms: %2").arg(newSubjectName,re.pattern()),
             QMessageBox::Ok,
             QMessageBox::NoButton
             );
        return;
    }

    ui->subject->setText(newSubjectName);
    if (newSubjectName.isEmpty())
        ui->changeSubjectButton->setText("Set");
    else
        ui->changeSubjectButton->setText("Change");

    qInfo() << "Subject set to" << newSubjectName;
}

void MainWindow::on_changeSubjectButton_clicked()
{
    QString newSubjectName = QInputDialog::getText(this, "Set subject", "Subject name:",  QLineEdit::Normal, QString(), NULL, Qt::CustomizeWindowHint);
    setSubjectName(newSubjectName);
}

bool MainWindow::setupSubjectDirectory()
{
    QString subject = ui->subject->text();

    if (subject.isEmpty()) {
        QString tmpSubjectName = QString::number(QDateTime::currentMSecsSinceEpoch()/1000);
        QMessageBox msgBox(this);
        QPushButton *continueButton = msgBox.addButton("Start Anyway", QMessageBox::ActionRole);
        QPushButton *addButton = msgBox.addButton("Add Subject", QMessageBox::ActionRole);
        QPushButton *cancelButton = msgBox.addButton("Cancel", QMessageBox::ActionRole);
        msgBox.setText(QString("Currently there is no test subject.\nIf record is started, subject will be set to %1.\t").arg(tmpSubjectName));
        msgBox.exec();
        if (msgBox.clickedButton() == continueButton)
            setSubjectName(tmpSubjectName);
        else if (msgBox.clickedButton() == addButton)
            on_changeSubjectButton_clicked();
        else if (msgBox.clickedButton() == cancelButton)
            return false;

        if (ui->subject->text().isEmpty()) // smartass user still entered an empty string, show him who's the boss
            setSubjectName(tmpSubjectName);
    }

    QString path = QDir::currentPath() + "/" + ui->subject->text();

    unsigned int recording = 0;
    if ( QDir( path ).exists() ) {
        QDirIterator it(path);
        while (it.hasNext()) {
            QRegExp re("[0-9]*");
            QString subDir = it.next();
            subDir = subDir.mid(path.size()+1);
            if (re.exactMatch(subDir)) {
                unsigned int subDirRecId = subDir.toInt();
                if (subDirRecId > recording)
                    recording = subDirRecId;
            }
        }
    }
    recording++;

    path += "/" + QString::number(recording);

    previousPwd = QDir().currentPath();
    QDir().mkpath(path);
    setWorkingDirectory(path);

    return true;
}

void MainWindow::on_recordingToggle_clicked()
{
    if (ui->recordingToggle->isChecked()) {
        if (!setupSubjectDirectory()) {
            ui->recordingToggle->setChecked(false);
            return;
        }
        qInfo() << "Record starting (Subject:" << ui->subject->text() << ")";
        ui->changeSubjectButton->setEnabled(false);
        ui->changePwdButton->setEnabled(false);
        emit startRecording();
        ui->recordingToggle->setText("Finish");
        connect(gazeEstimationWidget, SIGNAL(outDataTuple(DataTuple)),
                journal, SIGNAL(newData(DataTuple)) );
        QTimer::singleShot(500, this, SLOT(effectiveRecordingStart())); // TODO: right now we wait a predefined amount of time; ideally, we should wait for an ack from everyone involved
        ui->recordingToggle->setEnabled(false);
        recStartSound.play();
    } else {
        qInfo() << "Record stopped (Subject:" << ui->subject->text() << ")";
        emit stopRecording();
        disconnect(gazeEstimationWidget, SIGNAL(outDataTuple(DataTuple)),
                journal, SIGNAL(newData(DataTuple)) );
        killTimer(elapsedTimeUpdateTimer);
        elapsedTime.invalidate();
        ui->elapsedTime->setText("00:00:00");
        ui->recordingToggle->setText("Start");
        ui->blinker->hide();
        setWorkingDirectory(previousPwd);
        ui->changeSubjectButton->setEnabled(true);
        ui->changePwdButton->setEnabled(true);
        recStopSound.play();
    }
}
void MainWindow::effectiveRecordingStart()
{
    elapsedTime.restart();
    elapsedTimeUpdateTimer = startTimer(500);
    ui->recordingToggle->setEnabled(true);
    qInfo() << "Record started.";
}

void MainWindow::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == elapsedTimeUpdateTimer) {
        ui->elapsedTime->setText(QDateTime::fromTime_t((elapsedTime.elapsed()/1000+0.5), Qt::UTC).toString("hh:mm:ss"));

        if (ui->blinker->isVisible())
            ui->blinker->hide();
        else
            ui->blinker->show();
    }
}

void MainWindow::widgetButtonReact(QMainWindow *window, bool checked)
{
    if (!window)
        return;

    if (checked) {
        window->show();
        window->raise();
        window->setFocus();
    } else
        window->hide();
}

void MainWindow::on_leftEyeCam_clicked()
{
    widgetButtonReact(lEyeWidget, ui->leftEyeCam->isChecked());
}

void MainWindow::on_rightEyeCam_clicked()
{
    widgetButtonReact(rEyeWidget, ui->rightEyeCam->isChecked());
}

void MainWindow::on_fieldCam_clicked()
{
    widgetButtonReact(fieldWidget, ui->fieldCam->isChecked());
}

void MainWindow::on_gazeEstimation_clicked()
{
    widgetButtonReact(gazeEstimationWidget, ui->gazeEstimation->isChecked());
}

void MainWindow::on_log_clicked()
{
    widgetButtonReact(logWidget, ui->log->isChecked());
}

void MainWindow::on_performanceMonitor_clicked()
{
    widgetButtonReact(performanceMonitorWidget, ui->performanceMonitor->isChecked());
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat())
        return;
    switch (event->key()) {
        case Qt::Key_F:
            disconnect(gazeEstimationWidget, SIGNAL(outDataTuple(DataTuple)),
                fieldWidget, SLOT(preview(DataTuple)) );
            break;
        default:
            break;
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat())
        return;
    switch (event->key()) {
        case Qt::Key_F:
            connect(gazeEstimationWidget, SIGNAL(outDataTuple(DataTuple)),
                fieldWidget, SLOT(preview(DataTuple)) );
            break;
        default:
            break;
    }
}


void MainWindow::menuOption(QAction* action)
{
    if (action->text().toLower() == "references")
        showReferencesDialog();
    if (action->text().toLower() == "about")
        showAboutDialog();
}

void MainWindow::showReferencesDialog()
{
    ReferenceList::add( "Santini et al.",
        "EyeRecToo: Open-source Software for Real-time Pervasive Head-mounted Eye Tracking",
        "VISAPP", "2017a",
        "http://www.scitepress.org/DigitalLibrary/PublicationsDetail.aspx?ID=gLeoir7PxnI=&t=1"
    );
    ReferenceList::add( "Santini et al.",
        "CalibMe: Fast and Unsupervised Eye Tracker Calibration for Gaze-Based Pervasive Human-Computer Interaction",
        "CHI", "2017b",
        "http://confer.csail.mit.edu/chi2017/paper#!pn3994"
    );
    ReferenceList::add( "Fuhl et al.",
        "ElSe: Ellipse Selection for Robust Pupil Detection in Real-World Environments",
        "ETRA", "2016",
        "http://dl.acm.org/citation.cfm?id=2857505"
    );
    ReferenceList::add( "Fuhl et al.",
        "ExCuSe: Robust Pupil Detection in Real-World Scenarios",
        "LNCS", "2015",
        "https://link.springer.com/chapter/10.1007/978-3-319-23192-1_4"
    );
#ifdef STARBURST
    ReferenceList::add( "Li et al.",
        "Starburst: A Hybrid Algorithm for Video-based Eye Tracking Combining Feature-based and Model-based Approaches",
        "CVPR", "2005",
        "http://ieeexplore.ieee.org/abstract/document/1565386/"
    );
#endif
#ifdef SWIRSKI
    ReferenceList::add( "Swirski et al.",
        "Robust real-time pupil tracking in highly off-axis images",
        "ETRA", "2012",
        "http://dl.acm.org/citation.cfm?id=2168585"
    );
#endif
    ReferenceList::add( "Garrido-Jurado et al.",
        "Automatic generation and detection of highly reliable fiducial markers under occlusion",
        "Pattern Recognition", "2014",
        "http://dl.acm.org/citation.cfm?id=2589359"
    );
    ReferenceList::add( "Bradski et al.",
        "OpenCV",
        "Dr. Dobb’s Journal of Software Tools", "2000",
        "http://www.drdobbs.com/open-source/the-opencv-library/184404319"
    );
    ReferenceList::add( "Qt Project",
        "Qt Framework",
        "Online", "2017",
        "http://www.qt.io"
    );

    QString msg("EyeRecToo utilizes methods developed by multiple people. ");
    msg.append("This section provides a list of these methods so you can easily cite the ones you use :-)<br><br><br>");
    QMessageBox::information(this, "References", msg.append(ReferenceList::text()), QMessageBox::NoButton);
}

void MainWindow::showAboutDialog()
{
    QString msg = QString("EyeRecToo v%1<br><br>").arg(GIT_VERSION);
    msg.append("Contact: <a href=\"mailto:thiago.santini@uni-tuebingen.de?Subject=[EyeRecToo] Contact\" target=\"_top\">thiago.santini@uni-tuebingen.de</a><br><br>");
    msg.append("Copyright &copy; 2017 University of Tübingen");
    QMessageBox::about(this, "About", msg);
}

void MainWindow::setupWidget(QMainWindow *window, QPoint &position, const QSize &size, const bool &visible, QPushButton *button)
{
    // Sanitize position first
    bool inScreen = false;
    for (int i = 0; i < QApplication::desktop()->screenCount(); i++)
        inScreen |= QApplication::desktop()->screenGeometry(i).contains(position, true);

    if (!inScreen)
        position = QApplication::desktop()->screenGeometry().topLeft();

    window->move(position);
    window->resize(size);

    if (visible)
        window->show();

    if (button)
        button->setChecked(visible);
}
