#ifndef ERWIDGET_H
#define ERWIDGET_H

#include <QMainWindow>
#include <QObject>
#include <QSettings>

// We can't include utils.h here because of the global access used for the log
// and performance monitor widgets. So we define an alternative set function
// here for convenience
template<typename T> void reset(const QSettings *settings, const QString key, T &v)
{
	if (!settings)
		return;
	QVariant variant = settings->value(key);
	if (variant.isValid())
		v = qvariant_cast<T>(variant);
}

class ERWidget : public QMainWindow
{
	Q_OBJECT
public:
	explicit ERWidget(QString id, QWidget *parent = 0);

signals:
	void closed(bool b);
	void keyPress(QKeyEvent *event);
	void keyRelease(QKeyEvent *event);

public slots:

	void save(QSettings *settings)
	{
		settings->sync();
		settings->setValue(id + " Pos", pos());
		settings->setValue(id + " Size", size());
		settings->setValue(id + " Visible", isVisible());
	}

	void load(QSettings *settings)
	{
		QPoint pos = defaultPosition;
		QSize size = defaultSize;
		bool visible = defaultVisible;

		// Size sanity
		if (defaultSize.width() <= 0 || defaultSize.height() <= 0) {
			defaultSize = minimumSize();
			// If we don't have minimum sizes set in the form
			if (defaultSize.width() == 0)
				defaultSize.setWidth(320);
			if (defaultSize.height() == 0)
				defaultSize.setHeight(240);
		}

		settings->sync();
		reset(settings, id + " Pos",  pos);
		reset(settings, id + " Size",  size);
		reset(settings, id + " Visible",  visible);

		move(pos);
		resize(size);
		if (visible)
			this->show();
		else
			this->hide();
	}

	void setDefaults(bool visible = true, QSize size = {0,0}, QPoint pos = {0, 0})
	{
		defaultVisible = visible;
		defaultSize = size;
		defaultPosition = pos;
	}

	void setup() {
		setWindowTitle(id);
	}

protected:
	void hideEvent(QHideEvent *event) Q_DECL_OVERRIDE { Q_UNUSED(event) emit closed(false); }
	void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE { Q_UNUSED(event) emit closed(false); }
	void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE { emit keyPress(event); }
	void keyReleaseEvent(QKeyEvent *event) Q_DECL_OVERRIDE { emit keyRelease(event); }
	QString id;
	QSize defaultSize;
	bool defaultVisible;
	QPoint defaultPosition;

};

#endif // ERWIDGET_H
