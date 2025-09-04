#include "radarplot.h"
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

RadarPlot::RadarPlot(QWidget *parent)
    : QwtPolarPlot(parent), angle(0)
{
    setTitle("Radar Display");
    setPlotBackground(Qt::black);  // Qwt 6.x API

    QwtPolarGrid *grid = new QwtPolarGrid();
    grid->setPen(QPen(Qt::green, 0.5));
    grid->attach(this);

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &RadarPlot::updateScan);
}

void RadarPlot::startScan() { timer->start(30); }
void RadarPlot::stopScan()  { timer->stop(); }

void RadarPlot::addTarget(double angleDeg, double radiusRatio)
{
    targets.append(QPointF(angleDeg, radiusRatio));
}

void RadarPlot::updateScan()
{
    angle += 2;
    if (angle >= 360) angle = 0;
    replot();
}

void RadarPlot::drawCanvas(QPainter *painter, const QRectF &rect) const
{
    QwtPolarPlot::drawCanvas(painter, rect);

    if (!painter || rect.width() <= 0) return;

    QPointF center = rect.center();
    double radius = rect.width() / 2;

    int span = 60; // 扫描光角度宽
    for (int i = 0; i < span; i++)
    {
        double alpha = 255.0 * (span - i) / span;
        QColor color(0, 255, 0, alpha);

        QPainterPath path;
        path.moveTo(center);
        path.arcTo(rect, -angle + i, 1);
        path.closeSubpath();

        painter->setBrush(color);
        painter->setPen(Qt::NoPen);
        painter->drawPath(path);
    }

    painter->setBrush(Qt::red);
    for (const QPointF &t : targets)
    {
        if (t.y() <= 0) continue;
        double r = t.y() * radius;
        double theta = t.x() * M_PI / 180.0;
        QPointF pos(center.x() + r * cos(theta), center.y() - r * sin(theta));
        painter->drawEllipse(pos, 5, 5);
    }
}
