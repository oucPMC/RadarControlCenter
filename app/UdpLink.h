#pragma once
#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>

// 通过UDP协议进行网络通信，发送和接收报文，并通过信号通知接收到的数据

class UdpLink : public QObject {
    Q_OBJECT
public:
    explicit UdpLink(QObject* parent=nullptr);
     // 绑定本地地址和端口，开始监听 UDP 数据，返回 true 表示绑定成功，false 表示失败
    bool bind(const QHostAddress& addr, quint16 port);
    // 设置目标地址和端口，用于发送 UDP 数据
    void setPeer(const QHostAddress& host, quint16 port);
    // 发送数据到之前 setPeer 设置的目标地址，返回实际发送的字节数，如果失败则返回 -1
    qint64 send(const QByteArray& dat);

signals:
    // 当收到 UDP 数据包时发出该信号
    void packetReceived(QByteArray dat, QHostAddress from, quint16 port);

private slots:
    // 槽函数，当 sock_ 有数据可读时自动调用
    void onReadyRead();

private:
    QUdpSocket sock_;        // 内部的 UDP 套接字对象
    QHostAddress peer_;      // 目标主机地址
    quint16 peerPort_{0};    // 目标主机端口，默认为 0
};
