#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QUdpSocket>
#include "radarplot.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    QWidget *central;
    RadarPlot *radar;
    QPushButton *startButton;
    QPushButton *stopButton;
    QUdpSocket *udpSocket;
    QTimer *updateTimer;
    QMap<int, QJsonObject> radarDataCache; // 缓存雷达数据的容器
    // QTimer *simTimer;

private slots:
    void onRadarDataReceived();  // 处理接收到的雷达数据
    void startSimulation();      // 启动模拟
    void stopSimulation();       // 停止模拟
    void updateRadarData();      // 更新雷达数据并刷新显示
    // void onRadarDataParsed(const QByteArray &jsonData);
    // void simulateJsonData();
};

#endif // MAINWINDOW_H
