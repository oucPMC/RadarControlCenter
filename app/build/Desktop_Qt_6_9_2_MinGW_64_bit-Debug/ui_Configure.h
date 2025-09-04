/********************************************************************************
** Form generated from reading UI file 'Configure.ui'
**
** Created by: Qt User Interface Compiler version 6.9.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CONFIGURE_H
#define UI_CONFIGURE_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ConfigureClass
{
public:
    QWidget *centralWidget;
    QPushButton *pushButton;
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *ConfigureClass)
    {
        if (ConfigureClass->objectName().isEmpty())
            ConfigureClass->setObjectName("ConfigureClass");
        ConfigureClass->resize(600, 400);
        centralWidget = new QWidget(ConfigureClass);
        centralWidget->setObjectName("centralWidget");
        pushButton = new QPushButton(centralWidget);
        pushButton->setObjectName("pushButton");
        pushButton->setGeometry(QRect(270, 60, 81, 24));
        ConfigureClass->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(ConfigureClass);
        menuBar->setObjectName("menuBar");
        menuBar->setGeometry(QRect(0, 0, 600, 21));
        ConfigureClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(ConfigureClass);
        mainToolBar->setObjectName("mainToolBar");
        ConfigureClass->addToolBar(Qt::ToolBarArea::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(ConfigureClass);
        statusBar->setObjectName("statusBar");
        ConfigureClass->setStatusBar(statusBar);

        retranslateUi(ConfigureClass);

        QMetaObject::connectSlotsByName(ConfigureClass);
    } // setupUi

    void retranslateUi(QMainWindow *ConfigureClass)
    {
        ConfigureClass->setWindowTitle(QCoreApplication::translate("ConfigureClass", "Configure", nullptr));
        pushButton->setText(QCoreApplication::translate("ConfigureClass", "\345\220\257\345\212\250", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ConfigureClass: public Ui_ConfigureClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CONFIGURE_H
