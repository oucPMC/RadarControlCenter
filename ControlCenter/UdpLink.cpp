#include "UdpLink.h"

UdpLink::UdpLink(QObject* parent):QObject(parent) {
    connect(&sock_, &QUdpSocket::readyRead, this, &UdpLink::onReadyRead);
}

bool UdpLink::bind(const QHostAddress& addr, quint16 port) {
    return sock_.bind(addr, port, QUdpSocket::ShareAddress|QUdpSocket::ReuseAddressHint);
}

void UdpLink::setPeer(const QHostAddress& host, quint16 port) {
    peer_ = host; peerPort_ = port;
}

qint64 UdpLink::send(const QByteArray& dat) {
    if (peer_.isNull() || peerPort_==0) return -1;
    return sock_.writeDatagram(dat, peer_, peerPort_);
}

void UdpLink::onReadyRead() {
    while (sock_.hasPendingDatagrams()) {
        QByteArray dat;
        dat.resize(sock_.pendingDatagramSize());
        QHostAddress from; quint16 port;
        sock_.readDatagram(dat.data(), dat.size(), &from, &port);
        emit packetReceived(dat, from, port);
    }
}
