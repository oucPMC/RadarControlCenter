#pragma once
#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>

class UdpLink : public QObject {
    Q_OBJECT
public:
    explicit UdpLink(QObject* parent=nullptr);
    bool bind(const QHostAddress& addr, quint16 port);
    void setPeer(const QHostAddress& host, quint16 port);
    qint64 send(const QByteArray& dat);

signals:
    void packetReceived(QByteArray dat, QHostAddress from, quint16 port);

private slots:
    void onReadyRead();

private:
    QUdpSocket sock_;
    QHostAddress peer_;
    quint16 peerPort_{0};
};
