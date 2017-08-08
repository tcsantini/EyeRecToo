#ifndef LOGWIDGET_H
#define LOGWIDGET_H

#include <QMainWindow>

namespace Ui {
class LogWidget;
}

class LogWidget : public QMainWindow
{
    Q_OBJECT

public:
    explicit LogWidget(QWidget *parent = 0);
    ~LogWidget();

public slots:
    void appendMessage(const QString &msg);

private:
    Ui::LogWidget *ui;
};

#endif // LOGWIDGET_H
