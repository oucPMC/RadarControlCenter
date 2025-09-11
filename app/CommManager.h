#pragma once
#include <QObject>
#include <QTimer>
#include <QMap>
#include "UdpLink.h"
#include "Protocol.h"

// 正在等待 ACK 的命令
struct PendingCmd {
    QByteArray raw;   // 原始报文数据（需要重发时直接用）
    uint16_t cmdId;   // 报文 ID，用于 ACK 匹配和错误提示
    int retries;      // 已经重发的次数
    QTimer* timer;    // 定时器，等待 ACK 超时控制
};


class CommManager : public QObject {
    Q_OBJECT
public:
    explicit CommManager(QObject* parent=nullptr);

    // === 基础通信 ===
    void bind(const QHostAddress& local, quint16 port); // 绑定本机监听地址和端口
    void setPeer(const QHostAddress& ip, quint16 port); // 设置对端雷达的 IP 和端口

    // === 任务控制报文，控制雷达状态（待机/搜索/跟踪/展开撤收） ===
    void sendStandby();   // 0x1003
    void sendSearch();    // 0x1004
    void sendTrack();     // 0x1005
    void sendDeploy();    // 0x1008
    // 这些函数内部会构造对应报文，调用 sendWithAck()，等待 ACK。

    // === 参数配置报文 ===
    void sendSilentZone(uint16_t start, uint16_t end);  // 0x2091
    void sendIpConfig(const QString& ip, const QString& mask,
                      const QString& gw, quint16 port,
                      uint8_t dspMode=0, uint8_t dspId=0);  // 0x2081
    void sendPoseComp(double lat, double lon, float alt,
                      float yaw, float pitch, float roll,
                      float azComp, float elComp, float rngComp);  // 0x2011

    // === 查询报文，用于主动获取参数或状态 ===
    void query(uint16_t queryId);  // 0xF001

signals:
    void ackReceived(uint16_t cmdId, int result);   // 收到 ACK (0xF000)
    void commError(uint16_t cmdId);                 // 超时或失败
    void logMessage(const QString& msg);            // 日志输出
    void statusReceived(uint8_t workState, uint16_t range); // 收到状态报文 (0x3002)

private slots:
    void onPkt(QByteArray dat, QHostAddress from, quint16 port); // 处理底层 UdpLink 收到的报文，分发给 ACK/状态解析。
    void onTimeout(); // 定时器超时，触发重发或报错。

private:
    void sendWithAck(const QByteArray& raw, uint16_t cmdId, uint8_t seq);
    UdpLink link_;                              // 底层 UDP 通道
    QMap<uint8_t, PendingCmd> pending_;         // 等待 ACK 的命令，key=报文序号
};
