#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_Configure.h"
#include "CommManager.h"

class Configure : public QMainWindow
{
    Q_OBJECT

public:
    Configure(QWidget *parent = nullptr);
    ~Configure();

private slots:
    void onStartClicked();
    void onAck(uint16_t cmdId, int result);
    void onCommError(uint16_t cmdId);
    void onLogMessage(const QString& msg);
    void onStatus(uint8_t workState, uint16_t range);

private:
    Ui::ConfigureClass ui;
    CommManager* comm_;
};
