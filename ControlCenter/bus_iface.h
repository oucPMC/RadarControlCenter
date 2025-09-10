#ifndef BUS_IFACE_H
#define BUS_IFACE_H

#include <QtCore>
#include <QtNetwork>
#include <optional>
#include <functional>

/**
 * ͳһ����/���Ľӿڣ�������������ʵ�֣�
 * - �������豸�ġ�����/Ӧ��/��ѯ�ջ����������ϱ����Լ�ͨ����־�����
 * - MainWindow ��������ͷ�ļ����е������źŰ󶨣�����ʵ�ֿ��滻��
 *
 * ʹ��˼·��
 *  1) ��Ӧ������ʱ����һ�� IBus ʵ�֣��� class UdpBus : public IBus����
 *  2) MainWindow ���� configure()/setTargets()/start() �ȷ�����
 *  3) ��������/������������ sendWithAck()���ɹ�������� sendQuery() �γɱջ���
 *  4) ���� 0x3002 ���ϱ���ͨ�� onKeyStatusUpdated �źŸ��� UI ״̬����
 */

namespace BusIface {

    // =============== Э�鳣�������������ʾ���� ===============
    constexpr quint16 MSG_SET_SILENT_ZONE = 0x2091; // ���þ�Ĭ��
    constexpr quint16 MSG_QUERY_SILENT_ZONE = 0x2092; // ��ѯ��Ĭ��
    constexpr quint16 MSG_ACK = 0xF000; // ͨ��ACK
    constexpr quint16 MSG_STATUS_KEY_3002 = 0x3002; // �ؼ�״̬�ϱ����ײ�״̬����

    // ������ƣ�ʾ��ռλ�������Э�鲹��ʵ��ID��
    constexpr quint16 MSG_CMD_STANDBY = 0x1003;
    constexpr quint16 MSG_CMD_SEARCH = 0x1004;
    constexpr quint16 MSG_CMD_TRACK_START = 0x1101;
    constexpr quint16 MSG_CMD_TRACK_STOP = 0x1102;
    constexpr quint16 MSG_CMD_DEPLOY = 0x1011;
    constexpr quint16 MSG_CMD_RETRACT = 0x1012;

    // �������ã�ʾ��ռλ�������Э�鲹��ʵ��ID��
    constexpr quint16 MSG_CFG_IP = 0x2081;
    constexpr quint16 MSG_CFG_POSE = 0x2011;

    // =============== ֡ͷ�������� ===============
#pragma pack(push, 1)
    struct FrameHead {
        quint16 msgId;       // ����ID
        quint16 seq;         // ���
        quint32 payloadLen;  // ���س���
    };
#pragma pack(pop)

    static_assert(sizeof(FrameHead) == 8, "FrameHead must be 8 bytes");

    // =============== �������ݽṹ�������ͣ���ʵ�֣� ===============
    struct AckInfo {
        quint16 respondedMsgId{}; // ����Ӧ�ı���ID
        quint16 seq{};            // ��Ӧ�������
        quint16 resultCode{};     // 0=OK ����=������
        QString resultText;       // ��ѡ�ı�
    };

    struct SilentZoneParam {
        qint32 start_centideg{};  // 0.01��
        qint32 end_centideg{};    // 0.01��
    };

    struct KeyStatus3002 {
        quint16 workMode{};     // 0:���� 1:���� 2:���� 3:չ�� 4:���� ...
        quint16 trackCount{};   // ������
        qint16  temperature0{}; // 0.1��C
        quint16 healthFlags{};  // λ��־
    };

    // =============== ���в���/�ط�����/Ŀ��� ===============
    struct RetryPolicy {
        int ackTimeoutMs{ 1000 }; // ACK��ʱ
        int maxRetries{ 3 };      // ����ط�����
    };

    struct Target {
        QHostAddress ip;
        quint16 port{};
    };

    // =============== ʵ��ת������������������ʵ������ã� ===============
    qint32 degToCentideg(double deg);      // �������뵽 0.01��
    double centidegToDeg(qint32 centiDeg); // ת�ض�

    // =============== ͳһ�ӿڣ������ࣩ ===============
    class IBus : public QObject {
        Q_OBJECT
    public:
        explicit IBus(QObject* parent = nullptr) : QObject(parent) {}
        virtual ~IBus() = default;

        // --- ��������/���� ---
        virtual void configure(quint16 localListenPort,
            const RetryPolicy& policy) = 0;
        virtual void setTargets(const QList<Target>& targets) = 0;
        virtual bool start() = 0;   // �󶨶˿�/��ʼ����
        virtual void stop() = 0;    // �ر�

        // --- �������� ---
        // ֱ�ӷ���һ֡�����ȴ�ACK��
        virtual quint16 send(quint16 msgId, const QByteArray& payload,
            std::optional<quint16> seq = std::nullopt) = 0;

        // ���Ͳ��ȴ�ACK�ջ����ڲ�ִ�г�ʱ���ط���
        // onAckOk: ACK=0ʱ�ص�
        // onError: ��ʱ/������/����ʧ�ܵ�
        virtual quint16 sendWithAck(
            quint16 msgId,
            const QByteArray& payload,
            std::function<void(const AckInfo&)> onAckOk,
            std::function<void(const QString&)> onError,
            std::optional<quint16> seq = std::nullopt) = 0;

        // ���Ͳ�ѯ�౨�ģ��ȴ���Ӧ msgId ����Ӧ��
        virtual quint16 sendQuery(
            quint16 queryMsgId,
            const QByteArray& payload,
            std::function<void(quint16 /*respMsgId*/, const QByteArray& /*payload*/)> onResp,
            std::function<void(const QString&)> onError,
            std::optional<quint16> seq = std::nullopt) = 0;

        // --- �߲��ݷ�װ���� UI ֱ�ӵ��ã�ʵ�����ѡʵ�ֻ��׳��� ---
        // ���þ�Ĭ�����ɹ�ACK���Զ�����һ�� 0x2092 ��ѯ��ͨ�� signal ����
        virtual void setSilentZoneDeg(double startDeg, double endDeg) = 0;

        // ������ѯ��Ĭ����0x2092��
        virtual void querySilentZone() = 0;

        // �������
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

        // ����/ȡ������ 0x3002������ʵ��������/��ʱ��
        virtual void subscribe3002(bool enable) = 0;

    signals:
        // ԭʼ֪֡ͨ�����ڵ���/��־��
        void frameTx(quint16 msgId, quint16 seq, int payloadLen, QString targetsBrief);
        void frameRx(quint16 msgId, quint16 seq, int payloadLen, QString fromBrief);

        // ACK �¼�
        void ackArrived(const AckInfo& ack);

        // ��ѯ����¼����� 0x2092��
        void silentZoneUpdated(double startDeg, double endDeg);

        // ״̬������ؼ��ֶΣ�0x3002��
        void keyStatusUpdated(const KeyStatus3002& st);

        // ͳһ��־�У�UI ���ֱ��׷�ӣ�
        // time, cmd/ack id hex, seq, peer/target, result, note
        void logRow(QString timeStr, QString idHex, QString seqStr,
            QString peer, QString result, QString note);
    };

    // =============== ���������������������ɲ�ͬʵ���ṩ�� ===============
    /**
     * ����һ��Ĭ��ʵ�֣����� UDP ʵ�֣���
     * ��������û�����Ӿ���ʵ�֣��ɷ��� nullptr��
     */
    IBus* createDefaultBus(QObject* parent = nullptr);

} // namespace Bus

#endif // BUS_IFACE_H
