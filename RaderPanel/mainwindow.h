#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
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
};

#endif // MAINWINDOW_H
