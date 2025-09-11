#include "UdpLink.h"

UdpLink::UdpLink(QObject* parent):QObject(parent) {
    connect(&sock_, &QUdpSocket::readyRead, this, &UdpLink::onReadyRead);
}

// 绑定本地地址和端口
bool UdpLink::bind(const QHostAddress& addr, quint16 port) {
    return sock_.bind(addr, port, QUdpSocket::ShareAddress|QUdpSocket::ReuseAddressHint);
}

// 设置目标主机和端口，用于发送数据
void UdpLink::setPeer(const QHostAddress& host, quint16 port) {
    peer_ = host; peerPort_ = port;
}

// 发送数据到已设置的目标主机
qint64 UdpLink::send(const QByteArray& dat) {
    if (peer_.isNull() || peerPort_==0) return -1;
    return sock_.writeDatagram(dat, peer_, peerPort_);
}

// 处理接收到的 UDP 数据
void UdpLink::onReadyRead() {
    while (sock_.hasPendingDatagrams()) {
        QByteArray dat;
        // 调整 dat 大小以适应待接收的数据报
        dat.resize(sock_.pendingDatagramSize());
        QHostAddress from; quint16 port;
         // 读取数据报，同时获取发送方的地址和端口
        sock_.readDatagram(dat.data(), dat.size(), &from, &port);
        // 发出 packetReceived 信号，将数据和发送方信息传递出去
        emit packetReceived(dat, from, port);
    }
}
