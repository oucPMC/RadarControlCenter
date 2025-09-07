#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "radarplot.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() = default;

private:
    QWidget *central;
    RadarPlot *radar;
    QPushButton *startButton;
    QPushButton *stopButton;
    QTimer *simTimer;

private slots:
    void onRadarDataReceived(const QByteArray &jsonData);
    void simulateJsonData();
    void startSimulation();
    void stopSimulation();
};

#endif // MAINWINDOW_H
