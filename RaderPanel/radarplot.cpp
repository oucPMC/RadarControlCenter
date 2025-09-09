#include "radarplot.h"
#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QApplication>

RadarPlot::RadarPlot(QWidget *parent)
    : QwtPolarPlot(parent), angle(0)
{
    // 设置窗口最小大小
    setMinimumSize(600, 600);

    // 设置科技感背景 - 黑色背景
    setPlotBackground(Qt::black);

    // // 网格设置 - 使用绿色网格线
    // QwtPolarGrid *grid = new QwtPolarGrid();
    // QPen gridPen(QColor(0, 255, 100, 100), 1.0, Qt::DotLine);
    // grid->setPen(gridPen);

    // 启用方位和半径网格
    // grid->setGridAttribute(QwtPolarGrid::AxesGrid, true);
    // grid->setGridAttribute(QwtPolarGrid::MinorGrid, true);
    // 或者直接启用所有网格：
    // grid->showGrid(true);
    // grid->attach(this);

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
    // 先调用基类的绘制
    QwtPolarPlot::drawCanvas(painter, rect);

    if (!painter || rect.width() <= 0) return;

    QPointF center = rect.center();
    double radius = qMin(rect.width(), rect.height()) / 2.0 - 20;

    // 启用抗锯齿，确保圆环绘制平滑
    painter->setRenderHint(QPainter::Antialiasing, true);

    // 绘制雷达背景圆环
    for (int i = 1; i <= 5; ++i) {
        double ringRadius = radius * i / 5.0;
        painter->setPen(QPen(QColor(0, 255, 100, 180), 1));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(center, ringRadius, ringRadius);

        // 绘制距离刻度
        if (i > 0) {
            painter->setPen(QPen(QColor(0, 255, 100), 1));
            QString distanceText = QString::number(i * 400) + "m";
            // painter->drawText(center.x() + ringRadius - 20, center.y() - ringRadius - 5, distanceText);
            QRectF textRect(center.x() - 20, center.y() - ringRadius - 15, 40, 20);
            painter->drawText(textRect, Qt::AlignCenter, distanceText);

        }
    }

    // 绘制方位刻度
    painter->setPen(QPen(QColor(0, 255, 100), 1));
    for (int az = 0; az < 360; az += 30) {
        double rad = az * M_PI / 180.0;
        QPointF endPoint(center.x() + radius * qCos(rad), center.y() - radius * qSin(rad));
        painter->drawLine(center, endPoint);

        // 绘制方位文字
        QString azText = QString::number(az) + "°";
        QPointF textPoint(center.x() + (radius + 20) * qCos(rad),
                          center.y() - (radius + 20) * qSin(rad));
        painter->drawText(textPoint, azText);

    }

    // --- 扫描扇形光 ---
    int span = 30;

    for (int i = 0; i < span; ++i) {
        double alpha = 200.0 * (span - i) / span;
        QColor color(0, 255, 0, static_cast<int>(alpha));

        QPainterPath path;
        path.moveTo(center);
        path.arcTo(QRectF(center.x() - radius, center.y() - radius,
                          radius * 2, radius * 2),
                   -angle + i, 1);
        path.closeSubpath();

        painter->setBrush(color);
        painter->setPen(Qt::NoPen);
        painter->drawPath(path);
    }

    // --- 绘制轨迹 ---
    for (auto it = trackMap.constBegin(); it != trackMap.constEnd(); ++it) {
        const QList<TrackPoint> &track = it.value();
        if (track.isEmpty()) continue;

        QPointF prevPos;
        bool hasPrev = false;

        // 根据目标类型设置轨迹颜色（取该目标最后一个点的类型）
        QColor trackColor;
        int type = track.last().type;
        if (type == 1)      trackColor = QColor(255, 80, 80, 200);   // 无人机 → 红色
        else if (type == 2) trackColor = QColor(255, 255, 0, 200);   // 直升机 → 黄色
        else                trackColor = QColor(0, 200, 255, 200);   // 民航机 → 蓝色
        // 绘制轨迹线
        for (int i = 0; i < track.size(); ++i) {
            const TrackPoint &tp = track[i];
            double r = tp.distance * radius;
            double theta = tp.angle * M_PI / 180.0;
            QPointF pos(center.x() + r * qCos(theta), center.y() - r * qSin(theta));

            if (hasPrev) {
                painter->setPen(QPen(trackColor, 2));  // 使用分类颜色
                painter->drawLine(prevPos, pos);
            }
            prevPos = pos;
            hasPrev = true;
        }

        // 绘制最新目标点
        const TrackPoint &last = track.last();
        double rLast = last.distance * radius;
        double thetaLast = last.angle * M_PI / 180.0;
        QPointF lastPos(center.x() + rLast * qCos(thetaLast), center.y() - rLast * qSin(thetaLast));

        QColor color;
        QString typeText;
        if (last.type == 1) {
            color = QColor(255, 50, 50);
            typeText = "无人机";
        } else if (last.type == 2) {
            color = QColor(255, 255, 0);
            typeText = "直升机";
        } else {
            color = QColor(0, 150, 255);
            typeText = "民航机";
        }

        // 目标点发光效果
        painter->setBrush(QColor(color.red(), color.green(), color.blue(), 100));
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(lastPos, 12, 12);

        // 目标点实体
        painter->setBrush(color);
        painter->setPen(QPen(Qt::white, 1));
        painter->drawEllipse(lastPos, 5, 5);

        // 目标信息（多行显示）
        QString info = QString("%1\nLat: %2\nLon: %3\nV: %4 m/s")
                           .arg(typeText)
                           .arg(last.latitude, 0, 'f', 4)
                           .arg(last.longitude, 0, 'f', 4)
                           .arg(last.speed, 0, 'f', 1);

        painter->setPen(QColor(200, 255, 200));
        QFont infoFont("Microsoft YaHei", 8);
        painter->setFont(infoFont);

        // 给文本留一个 120x60 的矩形区域
        QRectF textRect(lastPos.x() + 15, lastPos.y() - 30, 120, 60);
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignTop, info);


    // 绘制雷达中心点
    painter->setBrush(QColor(0, 255, 100));
    painter->setPen(QPen(Qt::white, 1));
    painter->drawEllipse(center, 3, 3);
    }}
