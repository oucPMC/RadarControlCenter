#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "bus_iface.h"
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

QList<BusIface::Target> MainWindow::collectTargets() const {
    QList<BusIface::Target> out;
    for (int r = 0; r < m_targetsModel->rowCount(); ++r) {
        auto enIt = m_targetsModel->item(r, 0);
        auto ipIt = m_targetsModel->item(r, 1);
        auto poIt = m_targetsModel->item(r, 2);
        if (!ipIt || !poIt) continue;
        BusIface::Target t;
        t.enabled = enIt ? (enIt->checkState() == Qt::Checked) : true;
        t.ip = ipIt->text().trimmed();
        t.port = poIt->text().trimmed().toUShort();
        out << t;
    }
    return out;
}

QByteArray MainWindow::buildPayloadFromJson(const QVariantMap& j) const {
    // 统一占位实现：转 JSON 文本。
    QJsonObject o = QJsonObject::fromVariantMap(j);
    QJsonDocument doc(o);
    return doc.toJson(QJsonDocument::Compact);
}

void MainWindow::appendCmdRow(const QString& cmd, quint16 seq, const QString& targets,
    const QString& result, const QString& note) {
    QList<QStandardItem*> row;
    row << new QStandardItem(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
        << new QStandardItem(cmd)
        << new QStandardItem(seq == 0 ? "-" : QString::number(seq))
        << new QStandardItem(targets)
        << new QStandardItem(result)
        << new QStandardItem(note);
    m_cmdLogModel->appendRow(row);
}

void MainWindow::appendAckRow(quint16 ackId, quint16 respondedId, quint16 seq,
    quint16 result, const QString& note) {
    QList<QStandardItem*> row;
    row << new QStandardItem(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
        << new QStandardItem(QString("0x%1").arg(ackId, 4, 16, QChar('0')).toUpper())
        << new QStandardItem(QString("0x%1").arg(respondedId, 4, 16, QChar('0')).toUpper())
        << new QStandardItem(QString::number(seq))
        << new QStandardItem(QString::number(result))
        << new QStandardItem(note);
    m_ackLogModel->appendRow(row);
}


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

void MainWindow::onBusTx(quint16 msgId, quint16 seq, QList<BusIface::Target> targets, QString note) {
    QString t;
    for (int i = 0;i < targets.size();++i) {
        if (!targets[i].enabled) continue;
        if (!t.isEmpty()) t += "; ";
        t += QString("%1:%2").arg(targets[i].ip).arg(targets[i].port);
    }
    appendCmdRow(QString("0x%1").arg(msgId, 4, 16, QChar('0')).toUpper(), seq, t, "-", note);
}

void MainWindow::onBusAck(quint16 ackId, quint16 respondedId, quint16 seq, quint16 result, QString note) {
    appendAckRow(ackId, respondedId, seq, result, note);
}

void MainWindow::onBusPayload(quint16 msgId, quint16 seq, QByteArray payload) {
    // 演示：把 payload 试着当 JSON 显示到“参数查询表”
    QJsonParseError err{};
    auto doc = QJsonDocument::fromJson(payload, &err);
    if (err.error == QJsonParseError::NoError && doc.isObject()) {
        auto o = doc.object().toVariantMap();
        for (auto it = o.begin(); it != o.end(); ++it) {
            QList<QStandardItem*> row;
            row << new QStandardItem(it.key())
                << new QStandardItem(it.value().toString())
                << new QStandardItem("") << new QStandardItem(QString("0x%1 seq=%2").arg(msgId, 4, 16, QChar('0')).toUpper().arg(seq));
            m_queryModel->appendRow(row);
        }
    }
    else {
        // 若不是 JSON，就原样塞入一行
        QList<QStandardItem*> row;
        row << new QStandardItem(QString("MSG 0x%1").arg(msgId, 4, 16, QChar('0')).toUpper())
            << new QStandardItem(QString::fromUtf8(payload)))
            << new QStandardItem("") << new QStandardItem(QString("seq=%1").arg(seq));
        m_queryModel->appendRow(row);
    }
}

void MainWindow::onBus3002(QVariantMap f) {
    // 底部状态条关键字段
    sbWork->setText(QString("状态: %1").arg(f.value("workMode").toString("-")));
    sbFreq->setText(QString("频点: %1").arg(f.value("freq").toString("-")));
    sbSilent->setText(QString("静默: %1").arg(f.value("silent").toString("-")));
    sbGeo->setText(QString("经纬高: %1").arg(f.value("geo").toString("-")));
    sbAtt->setText(QString("姿态: %1").arg(f.value("att").toString("-")));
    sbAlarm->setText(QString("告警: %1").arg(f.value("alarm").toString("-")));
}


// ===== Toolbar slots =====
void MainWindow::onLoadConfig() {
    const QString file = QFileDialog::getOpenFileName(this, "导入配置", QString(), "JSON (*.json)");
    if (file.isEmpty()) return;
    QFile f(file);
    if (!f.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "错误", "无法读取文件");
        return;
    }
    const auto doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) { QMessageBox::warning(this, "错误", "JSON 格式不正确"); return; }
    const auto o = doc.object();

    // 连接配置
    ui->spinLocalPort->setValue(o.value("localPort").toInt(50000));
    ui->spinAckTimeoutMs->setValue(o.value("ackTimeoutMs").toInt(1000));
    ui->spinRetry->setValue(o.value("maxRetries").toInt(3));
    ui->comboCastMode->setCurrentIndex(o.value("castMode").toInt(0));

    // 目标列表
    m_targetsModel->removeRows(0, m_targetsModel->rowCount());
    const auto arr = o.value("targets").toArray();
    for (const auto& v : arr) {
        const auto t = v.toObject();
        const int row = m_targetsModel->rowCount();
        m_targetsModel->insertRow(row);
        auto en = new QStandardItem; en->setCheckable(true);
        en->setCheckState(t.value("enabled").toBool(true) ? Qt::Checked : Qt::Unchecked);
        en->setEditable(false);
        m_targetsModel->setItem(row, 0, en);
        m_targetsModel->setItem(row, 1, new QStandardItem(t.value("ip").toString("192.168.1.100")));
        m_targetsModel->setItem(row, 2, new QStandardItem(QString::number(t.value("port").toInt(62856))));
    }

    // 其它（可扩展：静默、位姿、IP 表等）
    m_ipListModel->removeRows(0, m_ipListModel->rowCount());
    const auto iparr = o.value("ipList").toArray();
    for (const auto& v : iparr) {
        const auto j = v.toObject();
        QList<QStandardItem*> row;
        row << new QStandardItem(j.value("dsp").toString())
            << new QStandardItem(j.value("ip").toString())
            << new QStandardItem(j.value("mask").toString())
            << new QStandardItem(j.value("gateway").toString())
            << new QStandardItem(QString::number(j.value("port").toInt()));
        m_ipListModel->appendRow(row);
    }

    QMessageBox::information(this, "提示", "配置已载入");
}

void MainWindow::onSaveConfig() {
    const QString file = QFileDialog::getSaveFileName(this, "保存配置", "config.json", "JSON (*.json)");
    if (file.isEmpty()) return;

    QJsonObject o;
    o["localPort"] = ui->spinLocalPort->value();
    o["ackTimeoutMs"] = ui->spinAckTimeoutMs->value();
    o["maxRetries"] = ui->spinRetry->value();
    o["castMode"] = ui->comboCastMode->currentIndex();

    QJsonArray arr;
    for (int r = 0;r < m_targetsModel->rowCount();++r) {
        QJsonObject t;
        auto en = m_targetsModel->item(r, 0);
        t["enabled"] = en ? (en->checkState() == Qt::Checked) : true;
        t["ip"] = m_targetsModel->item(r, 1)->text();
        t["port"] = m_targetsModel->item(r, 2)->text().toUShort();
        arr.append(t);
    }
    o["targets"] = arr;

    QJsonArray iparr;
    for (int r = 0;r < m_ipListModel->rowCount();++r) {
        QJsonObject j;
        j["dsp"] = m_ipListModel->item(r, 0)->text();
        j["ip"] = m_ipListModel->item(r, 1)->text();
        j["mask"] = m_ipListModel->item(r, 2)->text();
        j["gateway"] = m_ipListModel->item(r, 3)->text();
        j["port"] = m_ipListModel->item(r, 4)->text().toUShort();
        iparr.append(j);
    }
    o["ipList"] = iparr;

    QFile f(file);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, "错误", "无法写入文件");
        return;
    }
    f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
    f.close();
    QMessageBox::information(this, "提示", "配置已保存");
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
    if (!m_bus) { QMessageBox::warning(this, "错误", "通信接口未就绪"); return; }
    BusIface::Policy p;
    p.ackTimeoutMs = ui->spinAckTimeoutMs->value();
    p.maxRetries = ui->spinRetry->value();

    const quint16 localPort = ui->spinLocalPort->value();
    m_bus->setTargets(collectTargets());
    const bool ok = m_bus->start(localPort, p);
    sbConn->setText(ok ? "监听中" : "未连接");
    if (!ok) QMessageBox::warning(this, "错误", "启动监听失败");
}

void MainWindow::onStopListen() {
    if (m_bus) m_bus->stop();
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
    if (!m_bus) return;
    const quint16 MSG = 0x1003; // 待机（示例）
    QVariantMap j; j["mode"] = "standby";
    const auto seq = m_bus->sendCommand(MSG, buildPayloadFromJson(j));
    appendCmdRow("0x1003 待机", seq, "-", "-", "发送");
}

void MainWindow::onSearch() {
    if (!m_bus) return;
    const quint16 MSG = 0x1004; // 搜索（示例）
    QVariantMap j; j["mode"] = "search";
    const auto seq = m_bus->sendCommand(MSG, buildPayloadFromJson(j));
    appendCmdRow("0x1004 搜索", seq, "-", "-", "发送");
}

void MainWindow::onTrackStart() {
    if (!m_bus) return;
    const quint16 MSG = 0x1011; // 启动跟踪（示例）
    QVariantMap j;
    j["trackId"] = ui->editTrackId->text().trimmed();
    j["range_km"] = ui->spinRange->value();
    j["az_deg"] = ui->spinAz->value();
    j["el_deg"] = ui->spinEl->value();
    j["course_deg"] = ui->spinCourse->value();
    j["speed_mps"] = ui->spinSpeed->value();
    const auto seq = m_bus->sendCommand(MSG, buildPayloadFromJson(j));
    appendCmdRow("0x1011 跟踪开始", seq, "-", "-", "发送");
}

void MainWindow::onTrackStop() {
    if (!m_bus) return;
    const quint16 MSG = 0x1012; // 停止跟踪（示例）
    QVariantMap j; j["trackId"] = ui->editTrackId->text().trimmed();
    const auto seq = m_bus->sendCommand(MSG, buildPayloadFromJson(j));
    appendCmdRow("0x1012 跟踪停止", seq, "-", "-", "发送");
}

void MainWindow::onDeploy() {
    if (!m_bus) return;
    const quint16 MSG = 0x1021; // 展开（示例）
    QVariantMap j; j["action"] = "deploy";
    const auto seq = m_bus->sendCommand(MSG, buildPayloadFromJson(j));
    appendCmdRow("0x1021 展开", seq, "-", "-", "发送");
}

void MainWindow::onRetract() {
    if (!m_bus) return;
    const quint16 MSG = 0x1022; // 撤收（示例）
    QVariantMap j; j["action"] = "retract";
    const auto seq = m_bus->sendCommand(MSG, buildPayloadFromJson(j));
    appendCmdRow("0x1022 撤收", seq, "-", "-", "发送");
}

void MainWindow::onSendTrackParams() {
    if (!m_bus) return;
    const quint16 MSG = 0x1013; // 设置跟踪参数（示例）
    QVariantMap j;
    j["range_km"] = ui->spinRange->value();
    j["az_deg"] = ui->spinAz->value();
    j["el_deg"] = ui->spinEl->value();
    j["course_deg"] = ui->spinCourse->value();
    j["speed_mps"] = ui->spinSpeed->value();
    const auto seq = m_bus->sendCommand(MSG, buildPayloadFromJson(j));
    appendCmdRow("0x1013 跟踪参数", seq, "-", "-", "发送");
}


// ===== Query tab =====
void MainWindow::onQueryStatus() {
    if (!m_bus) return;
    m_queryModel->removeRows(0, m_queryModel->rowCount());
    const quint16 MSG = 0x3002; // 关键状态查询/订阅（按实现决定是否需要显式查询）
    QVariantMap j; j["query"] = "status";
    const auto seq = m_bus->sendCommand(MSG, buildPayloadFromJson(j));
    appendCmdRow("0x3002 状态查询", seq, "-", "-", "发送");
}

void MainWindow::onQueryParam() {
    if (!m_bus) return;
    // 根据下拉框选择不同的参数查询 ID
    const QString sel = ui->comboQueryId->currentText();
    quint16 msg = 0;
    if (sel.contains("静默区")) msg = 0x2092;
    else if (sel.contains("IP配置")) msg = 0x2082;
    else if (sel.contains("位姿补偿")) msg = 0x2012;
    else msg = 0x2000; // 兜底示例

    QVariantMap j; j["query"] = sel;
    const auto seq = m_bus->sendCommand(msg, buildPayloadFromJson(j));
    appendCmdRow(QString("0x%1 参数查询").arg(msg, 4, 16, QChar('0')).toUpper(), seq, "-", "-", "发送");
}

void MainWindow::onToggleSubscribe3002(bool checked) {
    ui->grpStatusMini->setEnabled(checked);
    if (m_bus) m_bus->subscribe(0x3002, checked);
}


// ===== Config tab =====
void MainWindow::onApplySilent() {
    if (!m_bus) { QMessageBox::warning(this, "错误", "通信接口未就绪"); return; }
    const double sDeg = ui->spinSilentStart->value();
    const double eDeg = ui->spinSilentEnd->value();
    // 这里仍然只通过统一接口传输抽象数据，不做具体单位换算与编码
    QVariantMap j; j["start_deg"] = sDeg; j["end_deg"] = eDeg;
    const auto seq = m_bus->sendCommand(0x2091, buildPayloadFromJson(j));
    appendCmdRow("0x2091 设置静默区", seq, "-", "-", "发送");
    // 闭环：立即查询
    const auto qseq = m_bus->sendCommand(0x2092, buildPayloadFromJson({ {"query","silent_zone"} }));
    appendCmdRow("0x2092 查询静默区", qseq, "-", "-", "发送");
}

void MainWindow::onApplyIp() {
    if (!m_bus) { QMessageBox::warning(this, "错误", "通信接口未就绪"); return; }
    QVariantMap cfg;
    cfg["modeIndex"] = ui->comboIpMode->currentIndex();
    cfg["dspIndex"] = ui->spinDspIndex->value();
    // 采集表格
    QVariantList list;
    for (int r = 0;r < m_ipListModel->rowCount();++r) {
        QVariantMap one;
        one["dsp"] = m_ipListModel->item(r, 0)->text();
        one["ip"] = m_ipListModel->item(r, 1)->text();
        one["mask"] = m_ipListModel->item(r, 2)->text();
        one["gateway"] = m_ipListModel->item(r, 3)->text();
        one["port"] = m_ipListModel->item(r, 4)->text().toUShort();
        list << one;
    }
    cfg["list"] = list;
    const auto seq = m_bus->sendCommand(0x2081, buildPayloadFromJson(cfg));
    appendCmdRow("0x2081 设置IP", seq, "-", "-", "发送");
    // 闭环查询
    const auto qseq = m_bus->sendCommand(0x2082, buildPayloadFromJson({ {"query","ip"} }));
    appendCmdRow("0x2082 查询IP", qseq, "-", "-", "发送");
}

void MainWindow::onApplyPose() {
    if (!m_bus) { QMessageBox::warning(this, "错误", "通信接口未就绪"); return; }
    // 读取界面：示例字段名，按你的 UI 控件名替换
    QVariantMap j;
    j["lat_deg"] = ui->spinLat->value();
    j["lon_deg"] = ui->spinLon->value();
    j["alt_m"] = ui->spinAlt->value();
    j["roll_deg"] = ui->spinRoll->value();
    j["pitch_deg"] = ui->spinPitch->value();
    j["yaw_deg"] = ui->spinYaw->value();
    j["east_m"] = ui->spinBiasE->value();
    j["north_m"] = ui->spinBiasN->value();


