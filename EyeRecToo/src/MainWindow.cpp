#include "MainWindow.h"
#include "ui_MainWindow.h"

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
    move(cfg.mainWindowPos);
    resize(cfg.mainWindowSize);

    if (!cfg.workingDirectory.isEmpty())
        setWorkingDirectory(cfg.workingDirectory);
    ui->pwd->setText(QDir::currentPath());

    ui->blinker->hide();

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
    lEyeWidget->move(cfg.leftEyeWidgetPos);
    lEyeWidget->resize(cfg.leftEyeWidgetSize);
    lEyeWidget->show();
    QThread::msleep(200);
    rEyeWidget = new CameraWidget("RightEye", ImageProcessor::Eye);
    rEyeWidget->setWindowIcon(QIcon(":/icons/rEyeWidget.png"));
    rEyeWidget->move(cfg.rightEyeWidgetPos);
    rEyeWidget->resize(cfg.rightEyeWidgetSize);
    rEyeWidget->show();
    QThread::msleep(200);
    fieldWidget = new CameraWidget("Field", ImageProcessor::Field);
    fieldWidget->setWindowIcon(QIcon(":/icons/fieldWidget.png"));
    fieldWidget->move(cfg.fieldWidgetPos);
    fieldWidget->resize(cfg.fieldWidgetSize);
    fieldWidget->show();

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
    gazeEstimationWidget->move(cfg.gazeEstimationWidgetPos);
    gazeEstimationWidget->resize(cfg.gazeEstimationWidgetSize);
    gazeEstimationWidget->show();
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

    checkForOpenH264();
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::closeEvent(QCloseEvent *event)
{
    if (ui->recordingToggle->isChecked())
        emit stopRecording();

    cfg.mainWindowPos = pos();
    cfg.mainWindowSize = size();
    cfg.leftEyeWidgetPos = lEyeWidget->pos();
    cfg.leftEyeWidgetSize = lEyeWidget->size();
    cfg.rightEyeWidgetPos = rEyeWidget->pos();
    cfg.rightEyeWidgetSize = rEyeWidget->size();
    cfg.fieldWidgetPos = fieldWidget->pos();
    cfg.fieldWidgetSize = fieldWidget->size();
    cfg.gazeEstimationWidgetPos = gazeEstimationWidget->pos();
    cfg.gazeEstimationWidgetSize = gazeEstimationWidget->size();
    cfg.workingDirectory = QDir::currentPath();
    if (settings)
        cfg.save(settings);

    qInfo() << "Closing LeftEyeWidget...";
    if ( lEyeWidget ) {
        lEyeWidget->close();
        lEyeWidget->deleteLater();
        lEyeWidget = NULL;
    }
    qInfo() << "Closing RightEyeWidget...";
    if ( rEyeWidget ) {
        rEyeWidget->close();
        rEyeWidget->deleteLater();
        rEyeWidget = NULL;
    }
    qInfo() << "Closing FieldWidget...";
    if ( fieldWidget ) {
        fieldWidget->close();
        fieldWidget->deleteLater();
        fieldWidget = NULL;
    }

    if (synchronizer)
        delete synchronizer;

    qInfo() << "Closing GazeEstimationWidget...";
    if (gazeEstimationWidget) {
        gazeEstimationWidget->close();
        gazeEstimationWidget->deleteLater();
    }

    qInfo() << "Stoping network stream...";
    if (networkStream)
        networkStream->deleteLater();

    if (settings)
        settings->deleteLater();

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
    } else {
        gRecording = false;
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
    }
}
void MainWindow::effectiveRecordingStart()
{
    elapsedTime.restart();
    elapsedTimeUpdateTimer = startTimer(500);
    gRecording = true;
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

void MainWindow::checkForOpenH264()
{
    QString openh264 = "unknown-platform-for-openh264";
#ifdef _WIN64
    openh264 = "openh264-1.6.0-win64msvc.dll";
#elif _WIN32
    openh264 = "openh264-1.4.0-win32msvc.dll";
#endif

#if __x86_64__ || __ppc64__
#if __linux__
    openh264 = "openh264-1.4.0-linux64.so";
#elif __APPLE__
    openh264 = "openh264-1.4.0-osx64.dylib";
#endif
#else
#if __linux__
    openh264 = "openh264-1.4.0-linux32.so";
#elif __APPLE__
    openh264 = "openh264-1.4.0-osx32.dylib";
#endif
#endif
    QString downloadURL = QString("http://ciscobinary.openh264.org/%1.bz2").arg(openh264);

    gHasOpenH264 = QFileInfo(gExeDir + "/" + openh264).exists();

    if (!gHasOpenH264 && cfg.showOpenH264Info) {
        int ret = QMessageBox::information(this, "EyeRecToo Video Encoding Info",
                                 QString("EyeRecToo favors using OpenH264 for video encoding.<br>")
                                 + QString("However, the user must download the library himself<br>")
                                 + QString("(why? <a href=\"%1\">%1</a>)<br><br>").arg("http://www.openh264.org/BINARY_LICENSE.txt")
                                 + QString("You can download it from<br><a href=\"%1\">%1</a><br> and unzip it at<br>%2<br>before continuing.<br>").arg(downloadURL).arg(gExeDir)
                                 + QString("Otherwise, defaulting to DivX.<br><br>")
                                 + QString("To disable OpenH264, simply delete<br>%1/%2").arg(gExeDir).arg(openh264)
                                 + QString("<br><br><br>%1").arg("OpenH264 Video Codec provided by Cisco Systems, Inc.<br><br><br>")
                                 + QString("Ok (always display this message) / Discard (never display this message)"),
                                   QMessageBox::Ok,
                                   QMessageBox::Discard);

        if (ret == QMessageBox::Discard)
            cfg.showOpenH264Info = false;

        gHasOpenH264 = QFileInfo(gExeDir + "/" + openh264).exists();
    }
    qInfo() << "Video encoder:" << (gHasOpenH264 ? "OpenH264" : "DivX");

}
