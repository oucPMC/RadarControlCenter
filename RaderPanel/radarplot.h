#ifndef RADARPLOT_H
#define RADARPLOT_H

#include <qwt_polar_plot.h>
#include <qwt_polar_grid.h>
#include <QTimer>
#include <QVector>
#include <QPointF>

class RadarPlot : public QwtPolarPlot
{
    Q_OBJECT
public:
    explicit RadarPlot(QWidget *parent = nullptr);
    void addTarget(double angleDeg, double radiusRatio);

public slots:
    void startScan();
    void stopScan();

protected:
    void drawCanvas(QPainter *painter, const QRectF &rect) const override;

private slots:
    void updateScan();

private:
    mutable int angle; // 当前扫描角度
    QTimer *timer;
    QVector<QPointF> targets;
};

#endif // RADARPLOT_H
