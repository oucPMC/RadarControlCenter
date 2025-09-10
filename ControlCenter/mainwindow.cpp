#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "Protocol.h"
#include "CommManager.h"
#include "PacketCodec.h"
#include "UdpLink.h"
#include <QLabel>
#include <QHeaderView>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QDateTime>
#include <QCheckBox>
#include <QAction>
#include <QPushButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setupModels();
    setupConnections();
    setupStatusBar();
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::setupModels() {
    // Targets table: Enable (checkable), IP, Port
    m_targetsModel = new QStandardItemModel(0, 3, this);
    m_targetsModel->setHorizontalHeaderLabels({QStringLiteral("启用"), QStringLiteral("IP"), QStringLiteral("端口")});
    ui->tableTargets->setModel(m_targetsModel);
    ui->tableTargets->horizontalHeader()->setStretchLastSection(true);
    ui->tableTargets->verticalHeader()->setVisible(false);
    ui->tableTargets->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableTargets->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

    // Command log
    m_cmdLogModel = new QStandardItemModel(0, 6, this);
    m_cmdLogModel->setHorizontalHeaderLabels({QStringLiteral("时间"), QStringLiteral("命令ID"), QStringLiteral("Seq"), QStringLiteral("目标"), QStringLiteral("结果"), QStringLiteral("备注")});
    ui->tableCmdLog->setModel(m_cmdLogModel);
    ui->tableCmdLog->horizontalHeader()->setStretchLastSection(true);
    ui->tableCmdLog->verticalHeader()->setVisible(false);
    ui->tableCmdLog->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableCmdLog->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Ack log
    m_ackLogModel = new QStandardItemModel(0, 6, this);
    m_ackLogModel->setHorizontalHeaderLabels({QStringLiteral("时间"), QStringLiteral("应答ID"), QStringLiteral("被响应ID"), QStringLiteral("Seq"), QStringLiteral("结果"), QStringLiteral("备注")});
    ui->tableAckLog->setModel(m_ackLogModel);
    ui->tableAckLog->horizontalHeader()->setStretchLastSection(true);
    ui->tableAckLog->verticalHeader()->setVisible(false);
    ui->tableAckLog->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableAckLog->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Query table: key, value, unit, desc
    m_queryModel = new QStandardItemModel(0, 4, this);
    m_queryModel->setHorizontalHeaderLabels({QStringLiteral("键"), QStringLiteral("值"), QStringLiteral("单位"), QStringLiteral("说明")});
    ui->tableQuery->setModel(m_queryModel);
    ui->tableQuery->horizontalHeader()->setStretchLastSection(true);
    ui->tableQuery->verticalHeader()->setVisible(false);
    ui->tableQuery->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableQuery->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // IP list: DSP, IP, Mask, Gateway, Port
    m_ipListModel = new QStandardItemModel(0, 5, this);
    m_ipListModel->setHorizontalHeaderLabels({QStringLiteral("DSP"), QStringLiteral("IP"), QStringLiteral("Mask"), QStringLiteral("Gateway"), QStringLiteral("Port")});
    ui->tableIpList->setModel(m_ipListModel);
    ui->tableIpList->horizontalHeader()->setStretchLastSection(true);
    ui->tableIpList->verticalHeader()->setVisible(false);
    ui->tableIpList->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableIpList->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
}

void MainWindow::setupConnections() {
    // Toolbar
    connect(ui->actLoadConfig, &QAction::triggered, this, &MainWindow::onLoadConfig);
    connect(ui->actSaveConfig, &QAction::triggered, this, &MainWindow::onSaveConfig);
    connect(ui->actConnect, &QAction::triggered, this, &MainWindow::onConnect);
    connect(ui->actDisconnect, &QAction::triggered, this, &MainWindow::onDisconnect);
    connect(ui->actClearLogs, &QAction::triggered, this, &MainWindow::onClearLogs);

    // Connection tab
    connect(ui->btnStart, &QPushButton::clicked, this, &MainWindow::onStartListen);
    connect(ui->btnStop, &QPushButton::clicked, this, &MainWindow::onStopListen);
    connect(ui->btnAddTarget, &QPushButton::clicked, this, &MainWindow::onAddTarget);
    connect(ui->btnRemoveTarget, &QPushButton::clicked, this, &MainWindow::onRemoveTarget);
    connect(ui->btnEnableAll, &QPushButton::clicked, this, &MainWindow::onEnableAllTargets);
    connect(ui->btnDisableAll, &QPushButton::clicked, this, &MainWindow::onDisableAllTargets);

    // Task tab
    connect(ui->btnStandby, &QPushButton::clicked, this, &MainWindow::onStandby);
    connect(ui->btnSearch, &QPushButton::clicked, this, &MainWindow::onSearch);
    connect(ui->btnTrackStart, &QPushButton::clicked, this, &MainWindow::onTrackStart);
    connect(ui->btnTrackStop, &QPushButton::clicked, this, &MainWindow::onTrackStop);
    connect(ui->btnDeploy, &QPushButton::clicked, this, &MainWindow::onDeploy);
    connect(ui->btnRetract, &QPushButton::clicked, this, &MainWindow::onRetract);
    connect(ui->btnSendTrackParams, &QPushButton::clicked, this, &MainWindow::onSendTrackParams);

    // Query tab
    connect(ui->btnQueryStatus, &QPushButton::clicked, this, &MainWindow::onQueryStatus);
    connect(ui->btnQueryParam, &QPushButton::clicked, this, &MainWindow::onQueryParam);
    connect(ui->chkSubscribe3002, &QCheckBox::toggled, this, &MainWindow::onToggleSubscribe3002);

    // Config tab
    connect(ui->btnApplySilent, &QPushButton::clicked, this, &MainWindow::onApplySilent);
    connect(ui->btnApplyIp, &QPushButton::clicked, this, &MainWindow::onApplyIp);
    connect(ui->btnApplyPose, &QPushButton::clicked, this, &MainWindow::onApplyPose);
}

void MainWindow::setupStatusBar() {
    auto mk = [](const QString& name)->QLabel*{
        QLabel* l = new QLabel; l->setObjectName(name); return l;
    };
    sbConn = mk("sbConn");
    sbWork = mk("sbWork");
    sbFreq = mk("sbFreq");
    sbSilent = mk("sbSilent");
    sbGeo = mk("sbGeo");
    sbAtt = mk("sbAtt");
    sbAlarm = mk("sbAlarm");

    statusBar()->addPermanentWidget(sbConn);
    statusBar()->addPermanentWidget(sbWork);
    statusBar()->addPermanentWidget(sbFreq);
    statusBar()->addPermanentWidget(sbSilent);
    statusBar()->addPermanentWidget(sbGeo);
    statusBar()->addPermanentWidget(sbAtt);
    statusBar()->addPermanentWidget(sbAlarm);

    sbConn->setText("未连接");
    sbWork->setText("状态: -");
    sbFreq->setText("频点: -");
    sbSilent->setText("静默: -");
    sbGeo->setText("经纬高: -");
    sbAtt->setText("姿态: -");
    sbAlarm->setText("告警: -");
}

// ===== Toolbar slots =====
void MainWindow::onLoadConfig() {
    // TODO: 读取 JSON，填充控件与表格
    const QString file = QFileDialog::getOpenFileName(this, "导入配置", QString(), "JSON (*.json)");
    if (file.isEmpty()) return;
    QMessageBox::information(this, "提示", "已选择配置文件: " + file + "\n(此处留给同学实现解析与应用)");
}

void MainWindow::onSaveConfig() {
    // TODO: 收集界面与模型数据，保存到 JSON
    const QString file = QFileDialog::getSaveFileName(this, "保存配置", "config.json", "JSON (*.json)");
    if (file.isEmpty()) return;
    QMessageBox::information(this, "提示", "将保存到: " + file + "\n(此处留给同学实现实际写入)");
}

void MainWindow::onConnect() { onStartListen(); }
void MainWindow::onDisconnect() { onStopListen(); }
void MainWindow::onClearLogs() {
    ui->txtConnLogBrief->clear();
    m_cmdLogModel->removeRows(0, m_cmdLogModel->rowCount());
    m_ackLogModel->removeRows(0, m_ackLogModel->rowCount());
}

// ===== Connection tab =====
void MainWindow::onStartListen() {
    // TODO: 读取 spinLocalPort/spinAckTimeoutMs/spinRetry/comboCastMode，启动socket
    sbConn->setText("监听中");
}

void MainWindow::onStopListen() {
    // TODO: 停止socket
    sbConn->setText("未连接");
}

void MainWindow::onAddTarget() {
    const int row = m_targetsModel->rowCount();
    m_targetsModel->insertRow(row);
    // Enable (checkable)
    QStandardItem *en = new QStandardItem;
    en->setCheckable(true);
    en->setCheckState(Qt::Checked);
    en->setEditable(false);
    m_targetsModel->setItem(row, 0, en);
    // IP
    m_targetsModel->setItem(row, 1, new QStandardItem("192.168.1.100"));
    // Port
    m_targetsModel->setItem(row, 2, new QStandardItem("62856"));
}

void MainWindow::onRemoveTarget() {
    auto sel = ui->tableTargets->selectionModel()->selectedRows();
    for (int i = sel.count() - 1; i >= 0; --i) m_targetsModel->removeRow(sel.at(i).row());
}

void MainWindow::onEnableAllTargets() {
    for (int r = 0; r < m_targetsModel->rowCount(); ++r) {
        auto it = m_targetsModel->item(r, 0);
        if (it) it->setCheckState(Qt::Checked);
    }
}

void MainWindow::onDisableAllTargets() {
    for (int r = 0; r < m_targetsModel->rowCount(); ++r) {
        auto it = m_targetsModel->item(r, 0);
        if (it) it->setCheckState(Qt::Unchecked);
    }
}

// ===== Task tab =====
void MainWindow::onStandby() {
    // TODO: 下发待机
    QList<QStandardItem*> row;
    row << new QStandardItem(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
        << new QStandardItem("0x1003 待机")
        << new QStandardItem("-")
        << new QStandardItem("-")
        << new QStandardItem("-")
        << new QStandardItem("占位");
    m_cmdLogModel->appendRow(row);
}

void MainWindow::onSearch() {
    // TODO
}

void MainWindow::onTrackStart() {
    // TODO: 读取 editTrackId/spinRange/spinAz/spinEl/spinCourse/spinSpeed
}

void MainWindow::onTrackStop() {
    // TODO
}

void MainWindow::onDeploy() {
    // TODO
}

void MainWindow::onRetract() {
    // TODO
}

void MainWindow::onSendTrackParams() {
    // TODO
}

// ===== Query tab =====
void MainWindow::onQueryStatus() {
    // TODO: 触发查询状态或依赖上报
    m_queryModel->removeRows(0, m_queryModel->rowCount());
    // 占位演示
    m_queryModel->appendRow({new QStandardItem("WorkState"), new QStandardItem("-"), new QStandardItem(""), new QStandardItem("0x3002")});
}

void MainWindow::onQueryParam() {
    // TODO: 根据 comboQueryId 的选择，发送“参数查询报文”
}

void MainWindow::onToggleSubscribe3002(bool checked) {
    // TODO: 控制 UI 刷新节流等
    ui->grpStatusMini->setEnabled(checked);
}

// ===== Config tab =====
void MainWindow::onApplySilent() {
    // TODO: 读取 spinSilentStart/End，下发 0x2091
    QMessageBox::information(this, "静默区", "将应用静默区配置（占位）。");
}

void MainWindow::onApplyIp() {
    // TODO: 读取 comboIpMode/spinDspIndex/tableIpList，下发 0x2081
    QMessageBox::information(this, "IP配置", "将应用IP配置（占位）。");
}

void MainWindow::onApplyPose() {
    // TODO: 读取位姿与补偿字段，下发 0x2011
    QMessageBox::information(this, "位置/补偿", "将应用位置/补偿（占位）。");
}
