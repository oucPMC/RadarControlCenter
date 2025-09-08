#pragma once
#include <QMainWindow>
#include <QStandardItemModel>
#include <QLabel>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
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
