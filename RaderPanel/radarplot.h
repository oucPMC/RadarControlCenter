#ifndef RADARPLOT_H
#define RADARPLOT_H

#include <qwt_polar_plot.h>
#include <qwt_polar_grid.h>
#include <QTimer>
#include <QMap>
#include <QList>
#include <QVector>
#include <QPointF>
#include <QString>

struct TrackPoint {
    double angle;
    double distance;
    double speed;
    double latitude;
    double longitude;
    int type;   // 目标类型：1=无人机, 2=直升机, 3=民航机
};

class RadarPlot : public QwtPolarPlot
{
    Q_OBJECT
public:
    explicit RadarPlot(QWidget *parent = nullptr);
    ~RadarPlot() override = default;

    void addTarget(double angleDeg, double radiusRatio);

public slots:
    void startScan();
    void stopScan();

    void updateTrack(int id, double angleDeg, double distanceRatio,
                     double speed, double latitude, double longitude, int type);

protected:
    void drawCanvas(QPainter *painter, const QRectF &rect) const override;

private slots:
    void updateScan();

private:
    int angle;                  // 扫描角度
    QTimer *timer;
    QVector<QPointF> targets;   // 测试点

    QMap<int, QList<TrackPoint>> trackMap; // 每个目标的轨迹
    static constexpr int MAX_LEN = 15;
};

#endif // RADARPLOT_H
