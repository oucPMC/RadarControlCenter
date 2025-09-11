#pragma once
#include "bus_iface.h"
#include "Protocol.h"
#include "PacketCodec.h"
#include <QUdpSocket>
#include <QTimer>
#include <QSet>

/**
 * @brief UdpBus：继承 BusIface，实现 UDP 通信
 *
 * 功能：
 *  - 绑定本地端口，维护目标地址
 *  - 发送命令报文（构造协议结构体 → 填写帧头 → 序列化 → 校验 → 发送）
 *  - 接收 UDP 报文并解析，发射 busTx/busAck/busPayload/bus3002 信号
 *  - 管理订阅列表（过滤是否转发到 UI）
 */
class UdpBus : public BusIface {
    Q_OBJECT
public:
    explicit UdpBus(QObject* parent = nullptr);
    ~UdpBus();

    void setTargets(const QList<Target>& targets) override;
    bool start(quint16 localPort, const Policy& policy) override;
    void stop() override;
    quint16 sendCommand(quint16 msgId, const QByteArray& payload) override;
    void subscribe(quint16 msgId, bool enable) override;

private slots:
    void onReadyRead(); // 接收到 UDP 报文时调用

private:
    QUdpSocket* socket{nullptr};
    QList<Target> targets;      // 下发目标
    Policy policy;              // 超时/重发策略
    quint16 seqCounter{1};      // 序列号生成器
    QSet<quint16> subscriptions;// 已订阅的报文 ID
};
