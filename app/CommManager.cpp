#include "CommManager.h"
#include "PacketCodec.h"
#include "Protocol.h"
#include <QDebug>

// 构造函数，实现连接 UdpLink::packetReceived 信号到 CommManager::onPkt 槽函数。
// 这样底层 UDP 一旦收到报文，就会调用 onPkt 来解析。
CommManager::CommManager(QObject* parent):QObject(parent) {
    connect(&link_, &UdpLink::packetReceived, this, &CommManager::onPkt);
}

// === 基础通信接口 ===
void CommManager::bind(const QHostAddress& local, quint16 port) {
    link_.bind(local, port);
}

void CommManager::setPeer(const QHostAddress& ip, quint16 port) {
    link_.setPeer(ip, port);
}

// === 任务控制 ===
void CommManager::sendStandby() {
    MsgCmdSimple pkt{};
    fillFrameHead(pkt.head, MSG_STANDBY, sizeof(pkt));
    pkt.check = crc16_modbus((const uint8_t*)&pkt, sizeof(pkt)-2);
    QByteArray raw = encode(pkt);
    sendWithAck(raw, MSG_STANDBY, pkt.head.seq);
}

void CommManager::sendSearch() {
    MsgCmdSimple pkt{};
    fillFrameHead(pkt.head, MSG_SEARCH, sizeof(pkt));
    pkt.check = crc16_modbus((const uint8_t*)&pkt, sizeof(pkt)-2);
    QByteArray raw = encode(pkt);
    sendWithAck(raw, MSG_SEARCH, pkt.head.seq);
}

void CommManager::sendTrack() {
    MsgCmdSimple pkt{};
    fillFrameHead(pkt.head, MSG_TRACK, sizeof(pkt));
    pkt.check = crc16_modbus((const uint8_t*)&pkt, sizeof(pkt)-2);
    QByteArray raw = encode(pkt);
    sendWithAck(raw, MSG_TRACK, pkt.head.seq);
}

void CommManager::sendDeploy() {
    MsgCmdSimple pkt{};
    fillFrameHead(pkt.head, MSG_DEPLOY, sizeof(pkt));
    pkt.check = crc16_modbus((const uint8_t*)&pkt, sizeof(pkt)-2);
    QByteArray raw = encode(pkt);
    sendWithAck(raw, MSG_DEPLOY, pkt.head.seq);
}

// === 参数配置报文 ===
// 静态区配置，构造静默区设置报文 (0x2091)
void CommManager::sendSilentZone(uint16_t start, uint16_t end) {
    MsgSilenceCfg pkt{};
    // 填充帧头
    fillFrameHead(pkt.head, MSG_SILENCE_CFG, sizeof(pkt));
    // 设置起点、终点
    pkt.start_001deg = start;
    pkt.end_001deg = end;
    size_t withoutTail=sizeof(pkt)-2;
    // 计算 CRC16
    pkt.check = crc16_modbus((const uint8_t*)&pkt, withoutTail);
    // 打包为 QByteArray
    QByteArray raw=encode(pkt);
    // 调用 sendWithAck()，发送并等待 ACK
    sendWithAck(raw, MSG_SILENCE_CFG, pkt.head.seq);
}

// IP 配置，构造 IP 配置报文 (0x2081)
void CommManager::sendIpConfig(const QString& ip, const QString& mask,
                               const QString& gw, quint16 port,
                               uint8_t dspMode, uint8_t dspId)
{
    MsgIpConfig pkt{};
    fillFrameHead(pkt.head, MSG_IP_CFG, sizeof(pkt));
    // 配置 DSP 模式、ID、IP、掩码、网关、端口
    pkt.dsp_mode = dspMode;
    pkt.dsp_id   = dspId;

    // IP 转换
    auto ipAddr   = QHostAddress(ip).toIPv4Address();
    auto maskAddr = QHostAddress(mask).toIPv4Address();
    auto gwAddr   = QHostAddress(gw).toIPv4Address();

    memcpy(pkt.ip,     &ipAddr,   4);
    memcpy(pkt.mask,   &maskAddr, 4);
    memcpy(pkt.gateway,&gwAddr,   4);

    pkt.port = port;

    // 校验后，广播到 255.255.255.255:0x1888
    pkt.check = crc16_modbus((const uint8_t*)&pkt, sizeof(pkt)-2);
    QByteArray raw = encode(pkt);
    link_.setPeer(QHostAddress::Broadcast, 0x1888);

    // 等待ack
    sendWithAck(raw, MSG_IP_CFG, pkt.head.seq);
}


// 误差补偿配置，构造姿态/补偿报文 (0x2051)：
void CommManager::sendPoseComp(double lat, double lon, float alt,
                               float yaw, float pitch, float roll,
                               float azComp, float elComp, float rngComp)
{
    MsgPoseComp pkt{};
    fillFrameHead(pkt.head, MSG_ERR_COMP_CFG, sizeof(pkt));
    // 配置误差补充报文的各种参数
    pkt.latitude = lat;
    pkt.longitude = lon;
    pkt.altitude = alt;
    pkt.yaw = yaw;
    pkt.pitch = pitch;
    pkt.roll = roll;
    pkt.sys_az_comp = azComp;
    pkt.sys_el_comp = elComp;
    pkt.sys_rng_comp = rngComp;
    // 校验后发送并等待 ACK
    pkt.check = crc16_modbus((const uint8_t*)&pkt, sizeof(pkt)-2);
    QByteArray raw = encode(pkt);
    sendWithAck(raw, MSG_ERR_COMP_CFG, pkt.head.seq);
}

// === 查询报文  ===
// 构造查询报文 (0xF001)，传入待查询的报文 ID，然后不等待 ACK 直接发出（因为查询报文会返回结果报文）
void CommManager::query(uint16_t queryId) {
    MsgQuery pkt{};
    fillFrameHead(pkt.head, MSG_QUERY, sizeof(pkt));
    pkt.query_id=queryId;
    size_t withoutTail=sizeof(pkt)-2;
    pkt.check = crc16_modbus((const uint8_t*)&pkt, withoutTail);
    QByteArray raw=encode(pkt);
    link_.send(raw);
}

// 发送并等待 ACK
void CommManager::sendWithAck(const QByteArray& raw, uint16_t cmdId, uint8_t seq) {
    // 发送报文
    link_.send(raw);
    auto* timer=new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(1000); // 1秒超时
    connect(timer, &QTimer::timeout, this, &CommManager::onTimeout);
    // 建立 PendingCmd，存入 pending_
    PendingCmd pc{raw,cmdId,0,timer};
    pending_[seq]=pc;
    // 启动定时器，1 秒后若未收到 ACK 则调用 onTimeout()
    timer->start();
    // 发送日志信号，可用于ui展示
    emit logMessage(tr("Sent cmd=0x%1 seq=%2").arg(cmdId,4,16,QChar('0')).arg(seq));
}

// 解析收到的报文
void CommManager::onPkt(QByteArray dat, QHostAddress, quint16) {
    // 校验报文合法性
    if (!verifyPacket(dat)) return;
    const FrameHead* h = reinterpret_cast<const FrameHead*>(dat.data());
    // 根据报文 ID 分类处理
    switch (h->msg_id_radar) {
    // 若为 ACK
    case MSG_ACK: {
        const MsgAck* ack = reinterpret_cast<const MsgAck*>(dat.data());
        uint8_t seq = h->seq;
        // 找到对应的 pending_
        if (pending_.contains(seq)) {
            auto pc = pending_.take(seq);
            // 停止定时器并清理
            pc.timer->stop();
            pc.timer->deleteLater();
            // 发送 ackReceived 信号
            emit ackReceived(ack->cmd_id, ack->result);
            // 演示性地发一次静默区查询（闭环演示）
            query(MSG_SILENCE_FB);
        }
        break;
    }
    // 若为状态报文
    case MSG_STATUS: {
        // 解析出工作状态 work_state 和 探测距离 detect_range_m
        const MsgStatus* st = reinterpret_cast<const MsgStatus*>(dat.data());
        // 发出 statusReceived 信号，用于ui展示
        emit statusReceived(st->work_state, st->detect_range_m);
        break;
    }
    default: break;
    }
}

// 超时重传接口
void CommManager::onTimeout() {
    QTimer* t=qobject_cast<QTimer*>(sender());
    if (!t) return;
    uint8_t seq=0xFF;
    // 遍历 pending_，找到超时的命令
    for (auto it=pending_.begin(); it!=pending_.end(); ++it) {
        if (it.value().timer==t) { seq=it.key(); break; }
    }
    if (seq==0xFF) return;
    auto pc=pending_[seq];
    // 若重试次数 < 3，则重发报文，计数 +1
    if (pc.retries<3) {
        link_.send(pc.raw);
        pending_[seq].retries++;
        pc.timer->start();
    }
    // 否则发送 commError 信号，并移除该序列
    else {
        emit commError(pc.cmdId);
        pending_.remove(seq);
        pc.timer->deleteLater();
    }
}


