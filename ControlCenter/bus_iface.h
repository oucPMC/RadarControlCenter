#ifndef BUS_IFACE_H
#define BUS_IFACE_H

#include <QtCore>
#include <QtNetwork>
#include <optional>
#include <functional>

/**
 * 统一总线/报文接口（仅声明，不含实现）
 * - 负责与设备的“发送/应答/查询闭环”、订阅上报、以及通用日志输出。
 * - MainWindow 仅依赖此头文件进行调用与信号绑定，具体实现可替换。
 *
 * 使用思路：
 *  1) 在应用启动时创建一个 IBus 实现（如 class UdpBus : public IBus）。
 *  2) MainWindow 调用 configure()/setTargets()/start() 等方法。
 *  3) 发送设置/控制类命令用 sendWithAck()，成功后按需调用 sendQuery() 形成闭环。
 *  4) 订阅 0x3002 等上报，通过 onKeyStatusUpdated 信号更新 UI 状态条。
 */

namespace BusIface {

    // =============== 协议常量（根据题干与示例） ===============
    constexpr quint16 MSG_SET_SILENT_ZONE = 0x2091; // 设置静默区
    constexpr quint16 MSG_QUERY_SILENT_ZONE = 0x2092; // 查询静默区
    constexpr quint16 MSG_ACK = 0xF000; // 通用ACK
    constexpr quint16 MSG_STATUS_KEY_3002 = 0x3002; // 关键状态上报（底部状态条）

    // 任务控制（示例占位，按你的协议补充实际ID）
    constexpr quint16 MSG_CMD_STANDBY = 0x1003;
    constexpr quint16 MSG_CMD_SEARCH = 0x1004;
    constexpr quint16 MSG_CMD_TRACK_START = 0x1101;
    constexpr quint16 MSG_CMD_TRACK_STOP = 0x1102;
    constexpr quint16 MSG_CMD_DEPLOY = 0x1011;
    constexpr quint16 MSG_CMD_RETRACT = 0x1012;

    // 参数配置（示例占位，按你的协议补充实际ID）
    constexpr quint16 MSG_CFG_IP = 0x2081;
    constexpr quint16 MSG_CFG_POSE = 0x2011;

    // =============== 帧头（网络序） ===============
#pragma pack(push, 1)
    struct FrameHead {
        quint16 msgId;       // 报文ID
        quint16 seq;         // 序号
        quint32 payloadLen;  // 负载长度
    };
#pragma pack(pop)

    static_assert(sizeof(FrameHead) == 8, "FrameHead must be 8 bytes");

    // =============== 解析数据结构（仅类型，不实现） ===============
    struct AckInfo {
        quint16 respondedMsgId{}; // 被响应的报文ID
        quint16 seq{};            // 对应请求序号
        quint16 resultCode{};     // 0=OK 其它=错误码
        QString resultText;       // 可选文本
    };

    struct SilentZoneParam {
        qint32 start_centideg{};  // 0.01°
        qint32 end_centideg{};    // 0.01°
    };

    struct KeyStatus3002 {
        quint16 workMode{};     // 0:待机 1:搜索 2:跟踪 3:展开 4:撤收 ...
        quint16 trackCount{};   // 跟踪数
        qint16  temperature0{}; // 0.1°C
        quint16 healthFlags{};  // 位标志
    };

    // =============== 运行参数/重发策略/目标等 ===============
    struct RetryPolicy {
        int ackTimeoutMs{ 1000 }; // ACK超时
        int maxRetries{ 3 };      // 最大重发次数
    };

    struct Target {
        QHostAddress ip;
        quint16 port{};
    };

    // =============== 实用转换声明（仅声明，供实现类调用） ===============
    qint32 degToCentideg(double deg);      // 四舍五入到 0.01°
    double centidegToDeg(qint32 centiDeg); // 转回度

    // =============== 统一接口（抽象类） ===============
    class IBus : public QObject {
        Q_OBJECT
    public:
        explicit IBus(QObject* parent = nullptr) : QObject(parent) {}
        virtual ~IBus() = default;

        // --- 生命周期/配置 ---
        virtual void configure(quint16 localListenPort,
            const RetryPolicy& policy) = 0;
        virtual void setTargets(const QList<Target>& targets) = 0;
        virtual bool start() = 0;   // 绑定端口/开始接收
        virtual void stop() = 0;    // 关闭

        // --- 基础发送 ---
        // 直接发送一帧（不等待ACK）
        virtual quint16 send(quint16 msgId, const QByteArray& payload,
            std::optional<quint16> seq = std::nullopt) = 0;

        // 发送并等待ACK闭环（内部执行超时与重发）
        // onAckOk: ACK=0时回调
        // onError: 超时/错误码/解析失败等
        virtual quint16 sendWithAck(
            quint16 msgId,
            const QByteArray& payload,
            std::function<void(const AckInfo&)> onAckOk,
            std::function<void(const QString&)> onError,
            std::optional<quint16> seq = std::nullopt) = 0;

        // 发送查询类报文（等待对应 msgId 的响应）
        virtual quint16 sendQuery(
            quint16 queryMsgId,
            const QByteArray& payload,
            std::function<void(quint16 /*respMsgId*/, const QByteArray& /*payload*/)> onResp,
            std::function<void(const QString&)> onError,
            std::optional<quint16> seq = std::nullopt) = 0;

        // --- 高层便捷封装（供 UI 直接调用；实现类可选实现或抛出） ---
        // 设置静默区：成功ACK后自动触发一次 0x2092 查询并通过 signal 发回
        virtual void setSilentZoneDeg(double startDeg, double endDeg) = 0;

        // 主动查询静默区（0x2092）
        virtual void querySilentZone() = 0;

        // 任务控制
        virtual void cmdStandby() = 0;
        virtual void cmdSearch() = 0;
        virtual void cmdTrackStart(const QString& trackId,
            double range_km,
            double az_deg,
            double el_deg,
            double course_deg,
            double speed_ms) = 0;
        virtual void cmdTrackStop(const QString& trackId) = 0;
        virtual void cmdDeploy() = 0;
        virtual void cmdRetract() = 0;
        virtual void cmdSendTrackParams(const QVariantMap& params) = 0;

        // 订阅/取消订阅 0x3002（可由实现做节流/定时）
        virtual void subscribe3002(bool enable) = 0;

    signals:
        // 原始帧通知（便于调试/日志）
        void frameTx(quint16 msgId, quint16 seq, int payloadLen, QString targetsBrief);
        void frameRx(quint16 msgId, quint16 seq, int payloadLen, QString fromBrief);

        // ACK 事件
        void ackArrived(const AckInfo& ack);

        // 查询结果事件（如 0x2092）
        void silentZoneUpdated(double startDeg, double endDeg);

        // 状态条所需关键字段（0x3002）
        void keyStatusUpdated(const KeyStatus3002& st);

        // 统一日志行（UI 表格直接追加）
        // time, cmd/ack id hex, seq, peer/target, result, note
        void logRow(QString timeStr, QString idHex, QString seqStr,
            QString peer, QString result, QString note);
    };

    // =============== 工厂方法（仅声明，可由不同实现提供） ===============
    /**
     * 创建一个默认实现（例如 UDP 实现）。
     * 若工程中没有链接具体实现，可返回 nullptr。
     */
    IBus* createDefaultBus(QObject* parent = nullptr);

} // namespace Bus

#endif // BUS_IFACE_H
