#pragma once
#include "bus_iface.h"    // ͳһͨ�Žӿ�
#include "CommManager.h"
#include <QMainWindow>
#include <QStandardItemModel>
#include <QLabel>

class CommManager;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // BusIface �źŶԽ�
    void onBusTx(quint16 msgId, quint16 seq, QList<BusIface::Target> targets, QString note);
    void onBusAck(quint16 ackId, quint16 respondedId, quint16 seq, quint16 result, QString note);
    void onBusPayload(quint16 msgId, quint16 seq, QByteArray payload);
    void onBus3002(QVariantMap fields);

    // Toolbar actions
    void onLoadConfig();
    void onSaveConfig();
    void onConnect();
    void onDisconnect();
    void onClearLogs();

    // Connection tab
    void onStartListen();
    void onStopListen();
    void onAddTarget();
    void onRemoveTarget();
    void onEnableAllTargets();
    void onDisableAllTargets();

    // Task tab
    void onStandby();
    void onSearch();
    void onTrackStart();
    void onTrackStop();
    void onDeploy();
    void onRetract();
    void onSendTrackParams();

    // Query tab
    void onQueryStatus();
    void onQueryParam();
    void onToggleSubscribe3002(bool checked);

    // Config tab
    void onApplySilent();
    void onApplyIp();
    void onApplyPose();

private:
    void setupModels();
    void setupConnections();
    void setupStatusBar();
    void appendQueryKV(const QString& key,
                       const QString& value,
                       const QString& unit = QString(),
                       const QString& desc = QString());

    BusIface* m_bus = nullptr;              // ͳһͨ�Žӿڣ��ⲿע��򹤳�������
    CommManager* m_commManager = nullptr;

    // === ���ߣ�UI <-> ���� ===
    QList<BusIface::Target> collectTargets() const;
    QByteArray buildPayloadFromJson(const QVariantMap& j) const; // ͳһ���л�ռλ
    void appendCmdRow(const QString& cmd, quint16 seq, const QString& targets, const QString& result, const QString& note);
    void appendAckRow(quint16 ackId, quint16 respondedId, quint16 seq, quint16 result, const QString& note);

    
    // Models for tables
    QStandardItemModel *m_targetsModel = nullptr;
    QStandardItemModel *m_cmdLogModel = nullptr;
    QStandardItemModel *m_ackLogModel = nullptr;
    QStandardItemModel *m_queryModel = nullptr;
    QStandardItemModel *m_ipListModel = nullptr;

    // Status bar labels
    QLabel *sbConn = nullptr;
    QLabel *sbWork = nullptr;
    QLabel *sbFreq = nullptr;
    QLabel *sbSilent = nullptr;
    QLabel *sbGeo = nullptr;
    QLabel *sbAtt = nullptr;
    QLabel *sbAlarm = nullptr;

    Ui::MainWindow *ui;
};
