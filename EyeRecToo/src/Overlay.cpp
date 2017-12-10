#include "Overlay.h"

void Overlay::drawEllipse(const cv::RotatedRect &ellipse)
{
	if (ellipse.size.width <= 0 || ellipse.size.height <= 0)
		return;

	painter.scale(scale.width(), scale.height());
	painter.translate(ellipse.center.x, ellipse.center.y);
	painter.rotate(ellipse.angle);
	painter.drawEllipse( -0.5*ellipse.size.width, -0.5*ellipse.size.height, ellipse.size.width, ellipse.size.height );
	painter.resetTransform();
}

void Overlay::drawROI(const QRectF &roi, const QColor &color)
{
	painter.setPen( QPen(color, 2, Qt::DotLine) );
	painter.setBrush( Qt::NoBrush );
	painter.drawRect(roi);
}

void EyeOverlay::drawPupil()
{
	if ( ! eyeData->validPupil || eyeData->pupil.confidence < minConfidence)
		return;

	float normalizedConfidence = (eyeData->pupil.confidence - minConfidence) / (1 - minConfidence);
	QColor color(255*(1-normalizedConfidence), 255*(normalizedConfidence), 0, alpha);

	// Pupil center
	painter.setPen(Qt::NoPen);
	painter.setBrush(color);

	cv::RotatedRect center = eyeData->pupil;
	float centerDiameter = 4*refPx;
	if (center.size.width > 0 && center.size.height > 0) {
		// Proportional
		if ( center.size.width > center.size.height ) {
			center.size.height = centerDiameter*(center.size.height / (float) center.size.width);
			center.size.width = centerDiameter;
		} else {
			center.size.width = centerDiameter*(center.size.width / (float) center.size.height);
			center.size.height = centerDiameter;
		}
	} else {
		// Fixed
		center.size.width = center.size.height = centerDiameter;
	}
	drawEllipse(center);

	// Pupil outline
	if (eyeData->pupil.size.width > 0 && eyeData->pupil.size.height > 0) {
		painter.setPen(QPen(color, refPx, Qt::SolidLine));
		painter.setBrush(Qt::NoBrush);
		drawEllipse(eyeData->pupil);
	}
}

void EyeOverlay::drawOverlay(const EyeData &data, const QRectF &userROI, const QRectF &coarseROI, QPaintDevice &paintDevice) {
	epilogue(data.input, paintDevice);
	eyeData = &data;
	drawROI(userROI, QColor(255,255,0,alpha) );
	drawROI(coarseROI, QColor(0,255,0,alpha) );
	drawPupil();
	prologue();
}

void FieldOverlay::drawGaze()
{
	if (!dataTuple->field.validGazeEstimate)
		return;

	painter.scale(scale.width(), scale.height());

	painter.setPen(QPen( QColor(0, 255, 0, alpha), 2*refPx, Qt::SolidLine));
	double radius = 5*refPx; // TODO: change based on evaluation error :-)
	if (radius > 0) {
		painter.drawEllipse(dataTuple->field.gazeEstimate.x - radius, dataTuple->field.gazeEstimate.y - radius , 2*radius, 2*radius);
		//painter.drawEllipse(dataTuple->field.gazeEstimate.x - radius, dataTuple->field.gazeEstimate.y - radius , 2*radius, 2*radius);
		//painter.drawLine(dataTuple->field.gazeEstimate.x, 0, dataTuple->field.gazeEstimate.x, dataTuple->field.width );
		//painter.drawLine(0, dataTuple->field.gazeEstimate.y, dataTuple->field.width, dataTuple->field.gazeEstimate.y );
	}

	painter.resetTransform();
}

void FieldOverlay::drawMarker(const Marker &marker)
{
	QPointF contour[4];
	for (unsigned int i=0; i<4; i++)
		contour[i] = QPointF(marker.corners[i].x, marker.corners[i].y);
	painter.drawConvexPolygon(contour, 4);
	int delta = 30*refPx;
	painter.drawText(
				QRectF(marker.center.x-delta/2, marker.center.y-delta/2, delta, delta),
				Qt::AlignCenter|Qt::TextWordWrap,
				QString("%1\n(%2)").arg(marker.id).arg(marker.center.z, 0, 'g', 2)
			);

}

void FieldOverlay::drawMarkers()
{
	painter.scale(scale.width(), scale.height());

	painter.setPen( QPen(QColor(0, 255, 255, alpha), 0.4*refPx, Qt::SolidLine) );
	font.setPointSize(4*refPx);
	painter.setFont(font);
	for (auto marker = dataTuple->field.markers.begin(); marker != dataTuple->field.markers.end(); marker++)
		if (dataTuple->field.collectionMarker.id != marker->id)
			drawMarker(*marker);
	if (dataTuple->field.collectionMarker.id != -1) {
		painter.setPen( QPen(QColor(0, 255, 0, alpha), 0.4*refPx, Qt::SolidLine) );
		drawMarker(dataTuple->field.collectionMarker);
	}

	painter.resetTransform();
}

void FieldOverlay::drawOverlay(const DataTuple &data, QPaintDevice &paintDevice) {
	cv::Mat input;
	if (data.showGazeEstimationVisualization && !data.gazeEstimationVisualization.empty())
		input = data.gazeEstimationVisualization;
	else
		input = data.field.input;
	epilogue(input, paintDevice);
	dataTuple = &data;
	drawMarkers();
	drawGaze();
	prologue();
}
