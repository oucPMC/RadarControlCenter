// bus_iface.h
#pragma once

#include <QObject>
#include <QList>
#include <QString>
#include <QByteArray>
#include <QVariantMap>

/**
 * @brief ����ͨ�����߽ӿڣ��� mainwindow.cpp ���ף�
 *
 * ���Ҫ�㣺
 * - ͳһ���������ڣ�sendCommand(msgId, payload) -> ���� seq
 * - ����/���ԣ�start/stop + setTargets + Policy
 * - ���ģ�subscribe(msgId, enable)������ 0x3002 ״̬��
 * - ͳһ�����¼���
 *      busTx(msgId, seq, targets, note)            �� �·�ʱ��¼
 *      busAck(ackId, respondedId, seq, result, note) �� �յ� ACK
 *      busPayload(msgId, seq, payload)             �� �յ�һ�㸺��
 *      bus3002(fields)                             �� �յ� 0x3002 �ؼ�״̬���ѽ���Ϊ QVariantMap��
 *
 * ˵����
 * - ���ļ���Ϊ����ӿڣ������ڶ��� .cpp ��ʵ�־��� UDP/���л�/ACK �ط��߼���
 * - �� mainwindow.cpp �е�ʹ���ϸ���룺�ṹ�塢�������ź�ǩ����ȫһ�¡�
 */
class BusIface : public QObject {
    Q_OBJECT
public:
    explicit BusIface(QObject* parent = nullptr) : QObject(parent) {}
    ~BusIface() override = default;

    // Ŀ��������ͬ mainwindow.cpp �� collectTargets()/onBusTx ʹ�ã�
    struct Target {
        bool    enabled{ true };
        QString ip;
        quint16 port{ 0 };
    };

    // ��ʱ/�ط����ԣ�ͬ onStartListen()��
    struct Policy {
        int ackTimeoutMs{ 1000 };
        int maxRetries{ 3 };
    };

    /**
     * @brief ��������Ŀ�֧꣨�ֶ�����Ƿ������� Target.enabled ���ƣ�
     */
    virtual void setTargets(const QList<Target>& targets) = 0;

    /**
     * @brief ��������/ͨ�š�
     * @param localPort ���������˿�
     * @param policy    ��ʱ/�ط�����
     * @return true �����ɹ���false ʧ��
     */
    virtual bool start(quint16 localPort, const Policy& policy) = 0;

    /**
     * @brief ֹͣͨ��/�ͷ���Դ
     */
    virtual void stop() = 0;

    /**
     * @brief �������ͳһ��ڣ�
     * @param msgId   ����ID������ 0x2091/0x2092/0x3002 ...��
     * @param payload ���أ����ϲ㹹�죬�˴������ľ�����룩
     * @return ���䲢���ص���� seq������ UI ����־�� ACK ƥ�䣩
     */
    virtual quint16 sendCommand(quint16 msgId, const QByteArray& payload) = 0;

    /**
     * @brief ����/�˶�ĳ���ϱ������� 0x3002��
     * @param msgId   ����ID
     * @param enable  ��/��
     */
    virtual void subscribe(quint16 msgId, bool enable) = 0;

signals:
    /**
     * @brief �·�ʱ�Ļص����� UI ��¼��������־����
     * @param note ��ѡ�ı�˵����ʵ�ֲ����䡰����/�ط�/�㲥������Ϣ��
     */
    void busTx(quint16 msgId, quint16 seq, QList<BusIface::Target> targets, QString note);

    /**
     * @brief �յ� ACK��0xF000�����֪ͨ
     * @param ackId        ͨ���̶�Ϊ 0xF000����ʵ�ֲ�����ֲ�ͬACK��
     * @param respondedId  ����Ӧ������ID������ UI ��ƥ����ʾ��
     * @param seq          ԭ��������
     * @param result       ����루0=�ɹ�������=�����룩
     * @param note         ��ע/�����ı�
     */
    void busAck(quint16 ackId, quint16 respondedId, quint16 seq, quint16 result, QString note);

    /**
     * @brief �յ�һ�㸺�أ����������ѯ�ķ��أ�
     */
    void busPayload(quint16 msgId, quint16 seq, QByteArray payload);

    /**
     * @brief 0x3002 �ؼ�״̬�ϱ������ѯ�������������Ϊ�ֶα�
     *  �����ֶμ��������飬��ǿ�ƣ���
     *   - workMode, freq, silent, geo, att, alarm ...
     */
    void bus3002(QVariantMap fields);
};


