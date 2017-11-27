#ifndef ERWIDGET_H
#define ERWIDGET_H

#include <QMainWindow>
#include <QObject>

class ERWidget : public QMainWindow
{
	Q_OBJECT
public:
	explicit ERWidget(QWidget *parent = 0);

signals:
	void closed(bool b);
	void keyPress(QKeyEvent *event);
	void keyRelease(QKeyEvent *event);

public slots:

protected:
	void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE { Q_UNUSED(event) emit closed(false); }
	void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE { emit keyPress(event); }
	void keyReleaseEvent(QKeyEvent *event) Q_DECL_OVERRIDE { emit keyRelease(event); }
};

#endif // ERWIDGET_H
