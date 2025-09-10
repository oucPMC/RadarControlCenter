#pragma once
#include <QObject>
#include <QTimer>
#include <QMap>
#include "UdpLink.h"
#include "Protocol.h"

struct PendingCmd {
    QByteArray raw;
    uint16_t cmdId;
    int retries;
    QTimer* timer;
};

class CommManager : public QObject {
    Q_OBJECT
public:
    explicit CommManager(QObject* parent=nullptr);
    void bind(const QHostAddress& local, quint16 port);
    void setPeer(const QHostAddress& ip, quint16 port);

    void sendSilentZone(uint16_t start, uint16_t end);
    void query(uint16_t queryId);

signals:
    void ackReceived(uint16_t cmdId, int result);
    void commError(uint16_t cmdId);
    void logMessage(const QString& msg);
    void statusReceived(uint8_t workState, uint16_t range);

private slots:
    void onPkt(QByteArray dat, QHostAddress from, quint16 port);
    void onTimeout();

private:
    void sendWithAck(const QByteArray& raw, uint16_t cmdId, uint8_t seq);
    UdpLink link_;
    QMap<uint8_t, PendingCmd> pending_;
};
