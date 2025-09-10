#include "CommManager.h"
#include "PacketCodec.h"
#include <QDebug>

CommManager::CommManager(QObject* parent):QObject(parent) {
    connect(&link_, &UdpLink::packetReceived, this, &CommManager::onPkt);
}

void CommManager::bind(const QHostAddress& local, quint16 port) {
    link_.bind(local, port);
}

void CommManager::setPeer(const QHostAddress& ip, quint16 port) {
    link_.setPeer(ip, port);
}

void CommManager::sendSilentZone(uint16_t start, uint16_t end) {
    MsgSilenceCfg pkt{};
    fillFrameHead(pkt.head, MSG_SILENCE_CFG, sizeof(pkt));
    pkt.start_001deg = start;
    pkt.end_001deg = end;
    size_t withoutTail=sizeof(pkt)-2;
    pkt.check = crc16_modbus((const uint8_t*)&pkt, withoutTail);
    QByteArray raw=encode(pkt);
    sendWithAck(raw, MSG_SILENCE_CFG, pkt.head.seq);
}

void CommManager::query(uint16_t queryId) {
    MsgQuery pkt{};
    fillFrameHead(pkt.head, MSG_QUERY, sizeof(pkt));
    pkt.query_id=queryId;
    size_t withoutTail=sizeof(pkt)-2;
    pkt.check = crc16_modbus((const uint8_t*)&pkt, withoutTail);
    QByteArray raw=encode(pkt);
    link_.send(raw);
}

void CommManager::sendWithAck(const QByteArray& raw, uint16_t cmdId, uint8_t seq) {
    link_.send(raw);
    auto* timer=new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(1000);
    connect(timer, &QTimer::timeout, this, &CommManager::onTimeout);
    PendingCmd pc{raw,cmdId,0,timer};
    pending_[seq]=pc;
    timer->start();
    emit logMessage(tr("Sent cmd=0x%1 seq=%2").arg(cmdId,4,16,QChar('0')).arg(seq));
}

// 解析
void CommManager::onPkt(QByteArray dat, QHostAddress, quint16) {
    if (!verifyPacket(dat)) return;
    const FrameHead* h = reinterpret_cast<const FrameHead*>(dat.data());
    switch (h->msg_id_radar) {
    case MSG_ACK: {
        const MsgAck* ack = reinterpret_cast<const MsgAck*>(dat.data());
        uint8_t seq = h->seq;
        if (pending_.contains(seq)) {
            auto pc = pending_.take(seq);
            pc.timer->stop(); pc.timer->deleteLater();
            emit ackReceived(ack->cmd_id, ack->result);
            // 收到 ACK 后查询静默区反馈 闭环仅展示
            query(MSG_SILENCE_FB);
        }
        break;
    }
    case 0x3002: { // 状态报文
        const MsgStatus* st = reinterpret_cast<const MsgStatus*>(dat.data());
        emit statusReceived(st->work_state, st->detect_range_m);
        break;
    }
    default: break;
    }
}

void CommManager::onTimeout() {
    QTimer* t=qobject_cast<QTimer*>(sender());
    if (!t) return;
    uint8_t seq=0xFF;
    for (auto it=pending_.begin(); it!=pending_.end(); ++it) {
        if (it.value().timer==t) { seq=it.key(); break; }
    }
    if (seq==0xFF) return;
    auto pc=pending_[seq];
    if (pc.retries<3) {
        link_.send(pc.raw);
        pending_[seq].retries++;
        pc.timer->start();
    } else {
        emit commError(pc.cmdId);
        pending_.remove(seq);
        pc.timer->deleteLater();
    }
}
