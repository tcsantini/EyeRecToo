#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QDir>
#include <QInputDialog>
#include <QFileDialog>
#include <QRegExp>
#include <QMessageBox>
#include <QElapsedTimer>
#include <QTimer>
#include <QDirIterator>
#include <QDesktopWidget>
#include <QDebug>

#include "CameraWidget.h"
#include "Synchronizer.h"

#include "GazeEstimationWidget.h"
#include "NetworkStream.h"

#include "Reference.h"

#include "LogWidget.h"
#include "PerformanceMonitorWidget.h"

#include "utils.h"

class MainWindowConfig
{
public:
    MainWindowConfig() :
    mainWindowPos( QPoint(0,0) ),
    mainWindowSize( QSize(100, 100)),
    logWidgetPos( QPoint(0,0) ),
    logWidgetSize( QSize(640, 240)),
    logWidgetVisible(true),
    leftEyeWidgetPos( QPoint(0,0) ),
    leftEyeWidgetSize( QSize(320,240) ),
    leftEyeWidgetVisible(true),
    rightEyeWidgetPos( QPoint(0,0) ),
    rightEyeWidgetSize( QSize(320, 240) ),
    rightEyeWidgetVisible(true),
    fieldWidgetPos( QPoint(0,0) ),
    fieldWidgetSize( QSize(320,240) ),
    fieldWidgetVisible(true),
    gazeEstimationWidgetPos( QPoint(0,0) ),
    gazeEstimationWidgetSize( QSize(100, 100) ),
    gazeEstimationWidgetVisible(true),
    performanceMonitorWidgetPos( QPoint(0,0) ),
    performanceMonitorWidgetSize( QSize(640, 240)),
    performanceMonitorWidgetVisible(true),
    workingDirectory("./")
    {}

    QPoint mainWindowPos;
    QSize mainWindowSize;
    QPoint logWidgetPos;
    QSize logWidgetSize;
    bool logWidgetVisible;
    QPoint leftEyeWidgetPos;
    QSize leftEyeWidgetSize;
    bool leftEyeWidgetVisible;
    QPoint rightEyeWidgetPos;
    QSize rightEyeWidgetSize;
    bool rightEyeWidgetVisible;
    QPoint fieldWidgetPos;
    QSize fieldWidgetSize;
    bool fieldWidgetVisible;
    QPoint gazeEstimationWidgetPos;
    QSize gazeEstimationWidgetSize;
    bool gazeEstimationWidgetVisible;
    QPoint performanceMonitorWidgetPos;
    QSize performanceMonitorWidgetSize;
    bool performanceMonitorWidgetVisible;
    QString workingDirectory;

    void save(QSettings *settings)
    {
        settings->sync();
        settings->setValue("mainWindowPos", mainWindowPos);
        settings->setValue("mainWindowSize", mainWindowSize);
        settings->setValue("logWidgetPos", logWidgetPos);
        settings->setValue("logWidgetSize", logWidgetSize);
        settings->setValue("logWidgetVisible", logWidgetVisible);
        settings->setValue("leftEyeWidgetPos", leftEyeWidgetPos);
        settings->setValue("leftEyeWidgetSize", leftEyeWidgetSize);
        settings->setValue("leftEyeWidgetVisible", leftEyeWidgetVisible);
        settings->setValue("rightEyeWidgetPos", rightEyeWidgetPos);
        settings->setValue("rightEyeWidgetSize", rightEyeWidgetSize);
        settings->setValue("rightEyeWidgetVisible", rightEyeWidgetVisible);
        settings->setValue("fieldWidgetPos", fieldWidgetPos);
        settings->setValue("fieldWidgetSize", fieldWidgetSize);
        settings->setValue("fieldWidgetVisible", fieldWidgetVisible);
        settings->setValue("gazeEstimationWidgetPos", gazeEstimationWidgetPos);
        settings->setValue("gazeEstimationWidgetSize", gazeEstimationWidgetSize);
        settings->setValue("gazeEstimationWidgetVisible", gazeEstimationWidgetVisible);
        settings->setValue("performanceMonitorWidgetPos", performanceMonitorWidgetPos);
        settings->setValue("performanceMonitorWidgetSize", performanceMonitorWidgetSize);
        settings->setValue("performanceMonitorWidgetVisible", performanceMonitorWidgetVisible);
        settings->setValue("workingDirectory", workingDirectory);
    }

    void load(QSettings *settings)
    {
        settings->sync();
        set(settings, "mainWindowPos", mainWindowPos);
        set(settings, "mainWindowSize", mainWindowSize);
        set(settings, "logWidgetPos", logWidgetPos);
        set(settings, "logWidgetSize", logWidgetSize);
        set(settings, "logWidgetVisible", logWidgetVisible);
        set(settings, "leftEyeWidgetPos", leftEyeWidgetPos);
        set(settings, "leftEyeWidgetSize", leftEyeWidgetSize);
        set(settings, "leftEyeWidgetVisible", leftEyeWidgetVisible);
        set(settings, "rightEyeWidgetPos", rightEyeWidgetPos);
        set(settings, "rightEyeWidgetSize", rightEyeWidgetSize);
        set(settings, "rightEyeWidgetVisible", rightEyeWidgetVisible);
        set(settings, "fieldWidgetPos", fieldWidgetPos);
        set(settings, "fieldWidgetSize", fieldWidgetSize);
        set(settings, "fieldWidgetVisible", fieldWidgetVisible);
        set(settings, "gazeEstimationWidgetPos", gazeEstimationWidgetPos);
        set(settings, "gazeEstimationWidgetSize", gazeEstimationWidgetSize);
        set(settings, "gazeEstimationWidgetVisible", gazeEstimationWidgetVisible);
        set(settings, "performanceMonitorWidgetPos", performanceMonitorWidgetPos);
        set(settings, "performanceMonitorWidgetSize", performanceMonitorWidgetSize);
        set(settings, "performanceMonitorWidgetVisible", performanceMonitorWidgetVisible);
        set(settings, "workingDirectory", workingDirectory);
    }

};

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

signals:
    void startRecording();
    void stopRecording();

private:
    Ui::MainWindow *ui;
    QSettings *settings;
    MainWindowConfig cfg;

    CameraWidget * lEyeWidget;
    CameraWidget * rEyeWidget;
    CameraWidget *fieldWidget;
    Synchronizer *synchronizer;
    GazeEstimationWidget *gazeEstimationWidget;
    QThread *journalThread;
    DataRecorderThread *journal;
    NetworkStream * networkStream;
    LogWidget *logWidget;
    PerformanceMonitorWidget *performanceMonitorWidget;

    QElapsedTimer elapsedTime;
    int elapsedTimeUpdateTimer;
    QString previousPwd;

    bool setupSubjectDirectory();
    void setSubjectName(QString newSubjectName);
    void setWorkingDirectory(QString dir);
    void widgetButtonReact(QMainWindow *window, bool checked);
    void createExtraMenus();
    void setupWidget(QMainWindow *window, QPoint &position, const QSize &size, const bool &visible=true, QPushButton *button=NULL);

protected:
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;

private slots:
    void on_changePwdButton_clicked();
    void on_changeSubjectButton_clicked();
    void on_recordingToggle_clicked();
    void timerEvent(QTimerEvent* event);
    void on_leftEyeCam_clicked();
    void on_rightEyeCam_clicked();
    void on_fieldCam_clicked();
    void on_gazeEstimation_clicked();

    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void effectiveRecordingStart();

    void menuOption(QAction*);
    void showReferencesDialog();
    void showAboutDialog();
    void on_log_clicked();
    void on_performanceMonitor_clicked();
};

#endif // MAINWINDOW_H
