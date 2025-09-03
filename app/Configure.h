#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_Configure.h"

class Configure : public QMainWindow
{
    Q_OBJECT

public:
    Configure(QWidget *parent = nullptr);
    ~Configure();

private:
    Ui::ConfigureClass ui;
};
