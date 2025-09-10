// bus_iface.h
#pragma once

#include <QObject>
#include <QList>
#include <QString>
#include <QByteArray>
#include <QVariantMap>

/**
 * @brief 抽象通信总线接口（与 mainwindow.cpp 配套）
 *
 * 设计要点：
 * - 统一的命令发送入口：sendCommand(msgId, payload) -> 返回 seq
 * - 连接/策略：start/stop + setTargets + Policy
 * - 订阅：subscribe(msgId, enable)（例如 0x3002 状态）
 * - 统一上行事件：
 *      busTx(msgId, seq, targets, note)            — 下发时记录
 *      busAck(ackId, respondedId, seq, result, note) — 收到 ACK
 *      busPayload(msgId, seq, payload)             — 收到一般负载
 *      bus3002(fields)                             — 收到 0x3002 关键状态（已解析为 QVariantMap）
 *
 * 说明：
 * - 该文件仅为抽象接口，方便在独立 .cpp 中实现具体 UDP/序列化/ACK 重发逻辑。
 * - 与 mainwindow.cpp 中的使用严格对齐：结构体、方法、信号签名完全一致。
 */
class BusIface : public QObject {
    Q_OBJECT
public:
    explicit BusIface(QObject* parent = nullptr) : QObject(parent) {}
    ~BusIface() override = default;

    // 目标主机（同 mainwindow.cpp 的 collectTargets()/onBusTx 使用）
    struct Target {
        bool    enabled{ true };
        QString ip;
        quint16 port{ 0 };
    };

    // 超时/重发策略（同 onStartListen()）
    struct Policy {
        int ackTimeoutMs{ 1000 };
        int maxRetries{ 3 };
    };

    /**
     * @brief 设置下行目标（支持多个；是否启用由 Target.enabled 控制）
     */
    virtual void setTargets(const QList<Target>& targets) = 0;

    /**
     * @brief 启动监听/通信。
     * @param localPort 本机监听端口
     * @param policy    超时/重发策略
     * @return true 启动成功；false 失败
     */
    virtual bool start(quint16 localPort, const Policy& policy) = 0;

    /**
     * @brief 停止通信/释放资源
     */
    virtual void stop() = 0;

    /**
     * @brief 发送命令（统一入口）
     * @param msgId   报文ID（例如 0x2091/0x2092/0x3002 ...）
     * @param payload 负载（由上层构造，此处不关心具体编码）
     * @return 分配并返回的序号 seq（用于 UI 侧日志与 ACK 匹配）
     */
    virtual quint16 sendCommand(quint16 msgId, const QByteArray& payload) = 0;

    /**
     * @brief 订阅/退订某类上报（例如 0x3002）
     * @param msgId   报文ID
     * @param enable  开/关
     */
    virtual void subscribe(quint16 msgId, bool enable) = 0;

signals:
    /**
     * @brief 下发时的回调（供 UI 记录“命令日志”）
     * @param note 可选文本说明（实现层可填充“发送/重发/广播”等信息）
     */
    void busTx(quint16 msgId, quint16 seq, QList<BusIface::Target> targets, QString note);

    /**
     * @brief 收到 ACK（0xF000）后的通知
     * @param ackId        通常固定为 0xF000（或实现层可区分不同ACK）
     * @param respondedId  被响应的命令ID（需与 UI 侧匹配显示）
     * @param seq          原命令的序号
     * @param result       结果码（0=成功，其他=错误码）
     * @param note         备注/错误文本
     */
    void busAck(quint16 ackId, quint16 respondedId, quint16 seq, quint16 result, QString note);

    /**
     * @brief 收到一般负载（例如参数查询的返回）
     */
    void busPayload(quint16 msgId, quint16 seq, QByteArray payload);

    /**
     * @brief 0x3002 关键状态上报（或查询结果），已整理为字段表
     *  典型字段键名（建议，不强制）：
     *   - workMode, freq, silent, geo, att, alarm ...
     */
    void bus3002(QVariantMap fields);
};


