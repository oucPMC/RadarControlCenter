#include "Configure.h"
#include <QHostAddress>

Configure::Configure(QWidget *parent)
    : QMainWindow(parent),
    comm_(new CommManager(this))
{
    ui.setupUi(this);

    connect(ui.pushButton, &QPushButton::clicked, this, &Configure::onStartClicked);
    connect(comm_, &CommManager::ackReceived, this, &Configure::onAck);
    connect(comm_, &CommManager::commError, this, &Configure::onCommError);
    connect(comm_, &CommManager::logMessage, this, &Configure::onLogMessage);
    connect(comm_, &CommManager::statusReceived, this, &Configure::onStatus);

    // 绑定指挥中心端口 (协议默认 0x1999)
    comm_->bind(QHostAddress::AnyIPv4, 0x1999);
    // 设置雷达目标地址 (协议默认 192.168.10.10:0x1888)
    comm_->setPeer(QHostAddress("192.168.10.10"), 0x1888);
}

Configure::~Configure() {}

void Configure::onStartClicked()
{
    // 下发静默区配置 0° ~ 90°
    comm_->sendSilentZone(0, 9000);
}

void Configure::onAck(uint16_t cmdId, int result)
{
    ui.statusBar->showMessage(tr("ACK 0x%1 result=%2").arg(cmdId, 4, 16, QLatin1Char('0')).arg(result));
}

void Configure::onCommError(uint16_t cmdId)
{
    ui.statusBar->showMessage(tr("Command 0x%1 timeout").arg(cmdId, 4, 16, QLatin1Char('0')));
}

void Configure::onLogMessage(const QString& msg)
{
    ui.statusBar->showMessage(msg);
}

void Configure::onStatus(uint8_t workState, uint16_t range)
{
    QString stateStr;
    switch (workState) {
    case 0x13: stateStr = "待机"; break;
    case 0x14: stateStr = "搜索"; break;
    case 0x15: stateStr = "跟踪"; break;
    default: stateStr = QString("0x%1").arg(workState,2,16,QChar('0'));
    }
    ui.statusBar->showMessage(tr("状态: %1 | 探测范围: %2 m").arg(stateStr).arg(range));
}
