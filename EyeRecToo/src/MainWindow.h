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

#include "ERWidget.h"

#include "CameraWidget.h"
#include "Synchronizer.h"

#include "GazeEstimationWidget.h"
#include "NetworkStream.h"

#include "Reference.h"

#include "LogWidget.h"
#include "PerformanceMonitorWidget.h"

#include "CommandManager.h"

#include "utils.h"

class MainWindowConfig
{
public:
    MainWindowConfig() :
	workingDirectory("./")
    {}

    void save(QSettings *settings)
    {
        settings->sync();
		settings->setValue("workingDirectory", workingDirectory);
    }

    void load(QSettings *settings)
    {
        settings->sync();
        set(settings, "workingDirectory", workingDirectory);
	}

	QString workingDirectory;

};

namespace Ui {
class MainWindow;
}

class MainWindow : public ERWidget
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
	CommandManager commandManager;

    QElapsedTimer elapsedTime;
    int elapsedTimeUpdateTimer;
    QString previousPwd;

    QSoundEffect recStartSound, recStopSound;

    bool setupSubjectDirectory();
    void setSubjectName(QString newSubjectName);
    void setWorkingDirectory(QString dir);
    void widgetButtonReact(QMainWindow *window, bool checked);
	void createExtraMenus();
	void setupWidget(ERWidget *widget, QSettings *settings, QPushButton *button=NULL);

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

	void effectiveRecordingStart();

    void menuOption(QAction*);
    void showReferencesDialog();
    void showAboutDialog();
    void on_log_clicked();
	void on_performanceMonitor_clicked();

	void toggleRemoteRecording();
	void freezeCameraImages();
    void unfreezeCameraImages();
    void togglePreview();

};

#endif // MAINWINDOW_H
