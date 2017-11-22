#ifndef OVERLAY_H
#define OVERLAY_H

#include <QPainter>

#include "ImageProcessor.h"
#include "EyeImageProcessor.h"
#include "FieldImageProcessor.h"
#include "Synchronizer.h"

class Overlay {
protected:
	Overlay() { font.setStyleHint(QFont::Monospace); }
	void epilogue(const cv::Mat &frame, QPaintDevice &paintDevice) {
		painter.begin(&paintDevice);
		scale = { paintDevice.width() / (float) frame.cols, paintDevice.height() / (float) frame.rows };
		refPx = 0.005f*std::max<float>(frame.cols, frame.rows);
	}
	void prologue() { painter.end(); }

	void drawEllipse(const cv::RotatedRect &ellipse);
	void drawROI(const QRectF &roi, const QColor &color);

	QPainter painter;
	QSizeF scale;
	QFont font;
	float refPx = 1;
	float alpha = 0.75f*255;
};

class EyeOverlay : private Overlay {
public:
	void drawOverlay(const EyeData &data, const QRectF &roi, const QRectF &coarseROI, QPaintDevice &paintDevice);

private:
	EyeData const *eyeData;
	void drawPupil();
	float minConfidence = 0.66f;
};

class FieldOverlay : private Overlay {
public:
	void drawOverlay(const DataTuple &data, QPaintDevice &paintDevice);

private:
	DataTuple const *dataTuple;
	void drawMarker(const Marker &marker);
	void drawMarkers();
	void drawGaze();
	float minConfidence = 0.66f;
};

#endif // OVERLAY_H
