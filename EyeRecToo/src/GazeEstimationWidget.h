#ifndef GAZEESTIMATIONWIDGET_H
#define GAZEESTIMATIONWIDGET_H

#include <vector>

#include <QMainWindow>
#include <QFileDialog>
#include <QThread>
#include <QKeyEvent>
#include <QSoundEffect>

#include "Synchronizer.h"
#include "GazeEstimation.h"

namespace Ui {
class GazeEstimationWidget;
}

class GazeEstimationWidget : public QMainWindow
{
    Q_OBJECT

public:
    explicit GazeEstimationWidget(QWidget *parent = 0);
    ~GazeEstimationWidget();

signals:
    void inDataTuple(DataTuple dataTuple);
    void outDataTuple(DataTuple dataTuple);
    void newClick(Timestamp timestamp, QPoint calibrationPoint, QSize previewSize);
    void resetCalibration(CollectionTuple::TupleType);
    void newTuple(CollectionTuple tuple);
    void newTuples(std::vector<CollectionTuple> tuples);
    void saveTuplesToFile(CollectionTuple::TupleType type, QString fileName);
    void loadTuplesFromFile(CollectionTuple::TupleType type, QString fileName);
    void calibrationRequest();
    void setCalibrating(bool v);
	void closed();

public slots:

private:
    QThread *gazeEstimationThread;
    Ui::GazeEstimationWidget *ui;
    GazeEstimation *gazeEstimation;
    GazeEstimationConfig cfg;
    QSettings *settings;

    CollectionTuple::TupleType currentTupleType;
    std::vector<CollectionTuple> samples;
    cv::Point2f fieldRatio;
    bool isCollecting;
    bool isSampling;
    bool lastStatus;
    bool calibrationRequested;

    QLabel *statusBarLabel;

    QSoundEffect startSound, successSound, failureSound, collectedSound;

private slots:
    void startSampling(Timestamp timestamp, QPoint calibrationPoint, QSize previewSize);
    void newSample(DataTuple dataTuple);
    void finishSampling();
    void collectMarkerTuple(DataTuple dataTuple);
    void updateStatus(bool status, QString msg);

    void on_saveTuples_clicked();
    void on_loadTuples_clicked();
    void on_startFinishButton_toggled(bool checked);
    void on_collectionTypeComboBox_currentIndexChanged(int index);

    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);

    void updateConfig();
    void on_samplingTimeMsCheckBox_editingFinished();
    void on_samplingMedianCheckBox_toggled(bool checked);
    void on_methodBox_currentIndexChanged(const QString &arg1);
    void on_inputTypeBox_currentIndexChanged(int index);
    void on_outlierRemovalBox_toggled(bool arg1);
    void on_outlierPupilRatioBox_toggled(bool checked);
    void on_outlierPupilPositionBox_toggled(bool checked);
    void on_outlierPupilOutlineBox_toggled(bool checked);
    void on_autoEvaluationBox_toggled(bool checked);
    void on_horizontalStrideSB_valueChanged(int arg1);
    void on_verticalStrideSB_valueChanged(int arg1);
    void on_granularitySB_valueChanged(int arg1);
    void on_rangeFactorSB_valueChanged(int arg1);
    void on_visualizationGroupBox_toggled(bool arg1);
	void on_visualizationTimeSpinBox_valueChanged(int arg1);

protected:
	void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE { Q_UNUSED(event) emit closed(); }
};

#endif // GAZEESTIMATIONWIDGET_H
