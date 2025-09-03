#include "Configure.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Configure w;
    w.show();
    return a.exec();
}
