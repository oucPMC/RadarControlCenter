#include "Udpbus.h"
#include <QHostAddress>
#include <QDebug>

UdpBus::UdpBus(QObject* parent) : BusIface(parent) {}

UdpBus::~UdpBus() {
    stop();
}

void UdpBus::setTargets(const QList<Target>& t) {
    targets = t;
}

/**
 * @brief 启动通信
 */
bool UdpBus::start(quint16 localPort, const Policy& p) {
    stop(); // 确保干净

    socket = new QUdpSocket(this);
    if (!socket->bind(QHostAddress::Any, localPort)) {
        qWarning() << "UDP bind failed on port" << localPort;
        delete socket;
        socket = nullptr;
        return false;
    }

    policy = p;
    connect(socket, &QUdpSocket::readyRead, this, &UdpBus::onReadyRead);

    qDebug() << "UdpBus started, listening on port" << localPort;
    return true;
}

/**
 * @brief 停止通信
 */
void UdpBus::stop() {
    if (socket) {
        socket->close();
        socket->deleteLater();
        socket = nullptr;
    }
}

/**
 * @brief 发送命令报文
 */
quint16 UdpBus::sendCommand(quint16 msgId, const QByteArray& payload) {
    if (!socket) {
        qWarning() << "sendCommand: socket not started";
        return 0;
    }

    quint16 seq = seqCounter++;
    if (seqCounter == 0) seqCounter = 1;

    // 选择合适的结构体
    QByteArray dat;
    switch (msgId) {
    case MSG_STANDBY:
    case MSG_SEARCH:
    case MSG_TRACK:
    case MSG_DEPLOY: {
        MsgCmdSimple pkt{};
        fillFrameHead(pkt.head, msgId, sizeof(pkt), CHECK_CRC16);
        dat = encode(pkt);
        break;
    }
    case MSG_SILENCE_CFG: {
        if (payload.size() < 4) { // start/end 两个 uint16
            qWarning() << "Invalid silence payload";
            return 0;
        }
        MsgSilenceCfg pkt{};
        pkt.start_001deg = *reinterpret_cast<const quint16*>(payload.constData());
        pkt.end_001deg   = *reinterpret_cast<const quint16*>(payload.constData()+2);
        fillFrameHead(pkt.head, msgId, sizeof(pkt), CHECK_CRC16);
        dat = encode(pkt);
        break;
    }
    default: {
        // 通用情况：直接把 payload 拼接到 FrameHead 后
        FrameHead h{};
        fillFrameHead(h, msgId, sizeof(FrameHead)+payload.size()+2, CHECK_CRC16);
        dat = QByteArray(reinterpret_cast<const char*>(&h), sizeof(h));
        dat.append(payload);
        dat.resize(dat.size()+2); // 给校验留两个字节
        break;
    }
    }

    // 写校验字段（最后两个字节）
    uint16_t check=0;
    if (!dat.isEmpty()) {
        const FrameHead* h = reinterpret_cast<const FrameHead*>(dat.constData());
        size_t withoutTail = dat.size()-2;
        if (h->check_mode==CHECK_CRC16)
            check=crc16_modbus((const uint8_t*)dat.data(), withoutTail);
        else if (h->check_mode==CHECK_SUM)
            check=sum16((const uint8_t*)dat.data(), withoutTail);
        memcpy(dat.data()+withoutTail, &check, 2);
    }

    // 广播到目标
    for (const auto& t : targets) {
        if (!t.enabled) continue;
        socket->writeDatagram(dat, QHostAddress(t.ip), t.port);
    }

    emit busTx(msgId, seq, targets, "发送");
    return seq;
}

/**
 * @brief 订阅/退订
 */
void UdpBus::subscribe(quint16 msgId, bool enable) {
    if (enable) {
        subscriptions.insert(msgId);
        qDebug() << "Subscribed" << msgId;
    } else {
        subscriptions.remove(msgId);
        qDebug() << "Unsubscribed" << msgId;
    }
}

/**
 * @brief 收到 UDP 报文
 */
void UdpBus::onReadyRead() {
    while (socket && socket->hasPendingDatagrams()) {
        QByteArray dat;
        dat.resize(int(socket->pendingDatagramSize()));
        QHostAddress from; quint16 port;
        socket->readDatagram(dat.data(), dat.size(), &from, &port);

        // 校验
        if (!verifyPacket(dat)) {
            qWarning() << "Invalid packet dropped";
            continue;
        }

        const FrameHead* h = reinterpret_cast<const FrameHead*>(dat.constData());
        quint16 msgId = h->msg_id_radar;
        quint16 seq   = h->seq;

        if (msgId == MSG_ACK) {
            const MsgAck* ack = reinterpret_cast<const MsgAck*>(dat.constData());
            emit busAck(MSG_ACK, ack->cmd_id, seq, ack->result, "ACK");
        } else if (msgId == MSG_STATUS) {
            const MsgStatus* st = reinterpret_cast<const MsgStatus*>(dat.constData());
            if (subscriptions.contains(MSG_STATUS)) {
                QVariantMap fields;
                fields["workMode"] = st->work_state;
                fields["range"]    = st->detect_range_m;
                fields["freq"]     = st->freq_GHz;
                fields["lat"]      = st->lat_deg;
                fields["lon"]      = st->lon_deg;
                emit bus3002(fields);
            }
        } else {
            if (subscriptions.contains(msgId)) {
                QByteArray payload = dat.mid(sizeof(FrameHead), dat.size()-sizeof(FrameHead)-2);
                emit busPayload(msgId, seq, payload);
            }
        }
    }
}
