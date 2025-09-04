#include "radarplot.h"
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

RadarPlot::RadarPlot(QWidget *parent)
    : QwtPolarPlot(parent), angle(0)
{
    setTitle("Radar Display");
    setPlotBackground(Qt::black);

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
    replot();
}

void RadarPlot::updateScan()
{
    angle += 2;
    if (angle >= 360) angle = 0;
    replot();
}

void RadarPlot::updateTrack(int id, double angleDeg, double distanceRatio,
                            double speed, double latitude, double longitude, int type)
{
    TrackPoint p{ angleDeg, distanceRatio, speed, latitude, longitude, type };
    trackMap[id].append(p);

    if (trackMap[id].size() > MAX_LEN) {
        trackMap[id].removeFirst();
    }
    replot();
}

void RadarPlot::drawCanvas(QPainter *painter, const QRectF &rect) const
{
    QwtPolarPlot::drawCanvas(painter, rect);

    if (!painter || rect.width() <= 0) return;

    QPointF center = rect.center();
    double radius = rect.width() / 2.0;

    // --- 扫描扇形光 ---
    int span = 60;
    for (int i = 0; i < span; ++i) {
        double alpha = 255.0 * (span - i) / span;
        QColor color(0, 255, 0, static_cast<int>(alpha));

        QPainterPath path;
        path.moveTo(center);
        path.arcTo(rect, -angle + i, 1);
        path.closeSubpath();

        painter->setBrush(color);
        painter->setPen(Qt::NoPen);
        painter->drawPath(path);
    }

    // --- 绘制轨迹 ---
    painter->setRenderHint(QPainter::Antialiasing, true);

    for (auto it = trackMap.constBegin(); it != trackMap.constEnd(); ++it) {
        const QList<TrackPoint> &track = it.value();
        if (track.isEmpty()) continue;

        QPointF prevPos;
        bool hasPrev = false;

        for (int i = 0; i < track.size(); ++i) {
            const TrackPoint &tp = track[i];
            double r = tp.distance * radius;
            double theta = tp.angle * M_PI / 180.0;
            QPointF pos(center.x() + r * qCos(theta), center.y() - r * qSin(theta));

            if (hasPrev) {
                painter->setPen(QPen(Qt::gray, 1));
                painter->drawLine(prevPos, pos);
            }
            prevPos = pos;
            hasPrev = true;
        }

        // 最新点
        const TrackPoint &last = track.last();
        double rLast = last.distance * radius;
        double thetaLast = last.angle * M_PI / 180.0;
        QPointF lastPos(center.x() + rLast * qCos(thetaLast), center.y() - rLast * qSin(thetaLast));

        QColor color;
        if (last.type == 1) color = Qt::red;
        else if (last.type == 2) color = Qt::yellow;
        else color = Qt::blue;

        painter->setBrush(color);
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(lastPos, 5, 5);

        QString info = QString("Lat:%1 Lon:%2 V:%3m/s")
                           .arg(last.latitude, 0, 'f', 4)
                           .arg(last.longitude, 0, 'f', 4)
                           .arg(last.speed, 0, 'f', 1);

        painter->setPen(Qt::white);
        QPointF textPos = lastPos + QPointF(8, -8);
        painter->drawText(textPos, info);
    }

    // 测试点
    if (!targets.isEmpty()) {
        painter->setBrush(Qt::blue);
        painter->setPen(Qt::NoPen);
        for (const QPointF &t : targets) {
            if (t.y() <= 0) continue;
            double r = t.y() * radius;
            double theta = t.x() * M_PI / 180.0;
            QPointF pos(center.x() + r * qCos(theta), center.y() - r * qSin(theta));
            painter->drawEllipse(pos, 4, 4);
        }
    }
}
