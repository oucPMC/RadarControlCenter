#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // 创建 centralWidget
    central = new QWidget(this);
    setCentralWidget(central);

    // 创建布局
    QVBoxLayout *layout = new QVBoxLayout(central);

    // 创建雷达控件
    radar = new RadarPlot(this);
    layout->addWidget(radar);

    //创建按钮
    startButton = new QPushButton("Start Scan", this);
    stopButton  = new QPushButton("Stop Scan", this);
    layout->addWidget(startButton);
    layout->addWidget(stopButton);

    // 连接信号槽
    connect(startButton, &QPushButton::clicked, radar, &RadarPlot::startScan);
    connect(stopButton,  &QPushButton::clicked, radar, &RadarPlot::stopScan);

    //添加目标点
    radar->addTarget(60, 0.7);
    radar->addTarget(120, 0.5);
    radar->addTarget(200, 0.6);

    // 可选：自动开始扫描
    // radar->startScan();
}
