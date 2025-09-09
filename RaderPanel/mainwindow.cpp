#include "mainwindow.h"
#include "TrackInfo.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <random>
#include <QStyleFactory>
#include <QPalette>
#include <QApplication>
#include <QLabel>
#include <QVBoxLayout>
#include <QUdpSocket>
#include <QtEndian>
#include <QByteArray>
#include <QDebug>
#include <QSpinBox>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QString neonStyle = R"(
    QWidget {
        background-color: #0a0a0a;
        color: #00ffaa;
        font-family: "Microsoft YaHei";
        font-size: 16pt;
    }

    QPushButton {
        color: #00ffaa;
        border: 2px solid #00ffaa;
        border-radius: 8px;
        padding: 6px 12px;
        background-color: rgba(0, 0, 0, 100);
    }
    QPushButton:hover {
        border: 2px solid #00ffcc;
        color: #00ffcc;
        background-color: rgba(0, 255, 170, 30);
    }

    QFrame, QGroupBox {
        margin-top: 10px;
    }
    QGroupBox::title {
        color: #00ffaa;
        subcontrol-origin: margin;
        left: 10px;
        padding: 0 3px;
    }

    QLabel {
        color: #00ffaa;
    }
)";

    qApp->setStyleSheet(neonStyle);

    // 设置暗色主题
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(20, 30, 40));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(15, 25, 35));
    darkPalette.setColor(QPalette::AlternateBase, QColor(20, 30, 40));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(30, 40, 50));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    QApplication::setPalette(darkPalette);

    // 设置窗口大小
    setMinimumSize(1000, 700);
    resize(800, 800);
    setWindowTitle("雷达控制系统");

    central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *layout = new QVBoxLayout(central);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    // 创建标签并添加边框，确保边框与文字的高度一致
    QLabel *label = new QLabel("雷达监控系统", this);
    label->setStyleSheet("font-size: 24pt; font-weight: bold; color: #00ffaa; "
                         "border: 2px solid #00ffaa; padding: 6px 12px;");
    label->setAlignment(Qt::AlignCenter); // 设置居中对齐

    // 在标签和雷达图之间添加间距
    layout->addWidget(label);

    // 创建雷达图
    radar = new RadarPlot(this);
    radar->setMinimumSize(600, 600);
    layout->addWidget(radar, 1);

    // 按钮样式
    QString buttonStyle =  "QPushButton {"
                          "   color: #00FFAA;"  // 字体颜色
                          "   border: 2px solid #00FFAA;"
                          "   border-radius: 8px;"
                          "   background-color: rgba(0, 0, 0, 80);"
                          "   font-weight: bold;"
                          "   padding: 8px;"
                          "   text-shadow: 0 0 8px #00FFAA;"  // 文字发光
                          "   box-shadow: 0 0 12px #00FFAA;"  // 边框发光
                          "}"
                          "QPushButton:hover {"
                          "   background-color: rgba(0, 255, 170, 30);"
                          "   border: 2px solid #00FFCC;"
                          "   color: #00FFCC;"
                          "   box-shadow: 0 0 20px #00FFCC;"
                          "}"
                          "QPushButton:pressed {"
                          "   background-color: rgba(0, 200, 150, 80);"
                          "   border: 2px solid #00DD99;"
                          "   color: #00DD99;"
                          "   box-shadow: 0 0 25px #00DD99;"
                          "}";

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    startButton = new QPushButton("开始扫描", this);
    stopButton  = new QPushButton("停止扫描", this);

    startButton->setStyleSheet(buttonStyle);
    stopButton->setStyleSheet(buttonStyle);

    startButton->setMinimumSize(240, 40);
    stopButton->setMinimumSize(240, 40);

    buttonLayout->addWidget(startButton);
    buttonLayout->addWidget(stopButton);
    buttonLayout->setAlignment(Qt::AlignCenter);
    buttonLayout->setSpacing(20);

    layout->addLayout(buttonLayout);

    connect(startButton, &QPushButton::clicked, this, &MainWindow::startSimulation);
    connect(stopButton,  &QPushButton::clicked, this, &MainWindow::stopSimulation);

    // simTimer = new QTimer(this);
    // connect(simTimer, &QTimer::timeout, this, &MainWindow::simulateJsonData);
}

MainWindow::~MainWindow() {
    if (updateTimer) {
        updateTimer->stop();
        delete updateTimer;
    }

    if(udpSocket) {
        udpSocket->close();
        delete udpSocket;
        udpSocket = nullptr;
    }
}

void MainWindow::startSimulation()
{
    //扇形扫描光
    radar->startScan();

    // 初始化UDP套接字
    udpSocket = new QUdpSocket(this);
    // 绑定UDP端口并开始接收数据
    udpSocket->bind(QHostAddress::Any, 65535); // 绑定接收端口
    connect(udpSocket, &QUdpSocket::readyRead, this, &MainWindow::onRadarDataReceived);

    // 启动500ms定时器
    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &MainWindow::updateRadarData);
    updateTimer->start(1200); // 每500ms触发一次
}

void MainWindow::stopSimulation()
{
    radar->stopScan();

    if (updateTimer) {
        updateTimer->stop();
        delete updateTimer;
        updateTimer = nullptr;
    }

    if(udpSocket) {
        udpSocket->close();
        delete udpSocket;
        udpSocket = nullptr;
    }
}

void MainWindow::updateRadarData()
{
    // 在此函数中更新雷达图的显示
    for (auto it = radarDataCache.begin(); it != radarDataCache.end(); ++it) {
        int id = it.key();
        QJsonObject targetObj = it.value()["radar"].toObject()["targets"].toArray()[0].toObject();

        double azimuth = targetObj["azimuth"].toDouble();
        double distance = targetObj["distance"].toDouble();
        while(distance > 2000) {
            distance -= 2000;
        }
        double speed = targetObj["speed"].toDouble();
        double latitude = targetObj["latitude"].toDouble();
        double longitude = targetObj["longitude"].toDouble();
        int type = targetObj["type"].toInt();

        radar->updateTrack(id, azimuth, distance / MAX_RANGE, speed, latitude, longitude, type);
    }
}


// 用于解析数据的槽函数
void MainWindow::onRadarDataReceived() {
    // 检查是否有数据可读取
    while (udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());

        QHostAddress sender;
        quint16 senderPort;

        // 接收数据
        udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        qDebug() << "Received datagram: " << datagram.toHex();

        // 解析数据
        if (datagram.size() == 142) {  // 确保数据足够长
            QByteArray trackData = datagram.mid(53, 71);  // 解析航迹数据

            // 手动解析数据
            TrackInfo track;

            // 从 trackData 中提取字段数据
            QDataStream stream(trackData);
            stream.setByteOrder(QDataStream::LittleEndian);

            // 手动读取每个字段
            track.trackBatchNumber = qFromLittleEndian<quint16>(trackData.mid(0, 2));
            track.targetLongitude = qFromLittleEndian<double>(trackData.mid(2, 8));
            track.targetLatitude = qFromLittleEndian<double>(trackData.mid(10, 8));
            track.targetAltitude = qFromLittleEndian<float>(trackData.mid(18, 4));
            track.distance = qFromLittleEndian<float>(trackData.mid(22, 4));
            track.azimuth = qFromLittleEndian<float>(trackData.mid(26, 4));
            track.pitch = qFromLittleEndian<float>(trackData.mid(30, 4));
            track.targetSpeed = qFromLittleEndian<float>(trackData.mid(34, 4));
            track.targetHeading = qFromLittleEndian<float>(trackData.mid(38, 4));
            track.targetStrength = qFromLittleEndian<float>(trackData.mid(42, 4));
            track.reserved1 = trackData.at(46);
            track.reserved2 = trackData.at(47);
            track.reserved3 = trackData.at(48);
            track.reserved4 = trackData.at(49);
            track.targetType = trackData.at(50);
            track.targetSize = trackData.at(51);
            track.trackPointType = trackData.at(52);
            track.trackType = trackData.at(53);
            track.consecutiveLostCount = trackData.at(54);
            track.trackQuality = trackData.at(55);
            track.rawDistance = qFromLittleEndian<float>(trackData.mid(56, 4));
            track.rawAzimuth = qFromLittleEndian<float>(trackData.mid(60, 4));
            track.rawPitch = qFromLittleEndian<float>(trackData.mid(64, 4));
            track.reserved5 = trackData.at(68);
            track.reserved6 = trackData.at(69);
            track.reserved7 = trackData.at(70);

            // 打印解析结果
            qDebug() << "Track Info - "
                     << "ID:" << track.trackBatchNumber
                     << "Azimuth:" << track.azimuth
                     << "Distance:" << track.distance
                     << "Speed:" << track.targetSpeed
                     << "Lat:" << track.targetLatitude
                     << "Lon:" << track.targetLongitude
                     << "Type:" << track.targetType;

            // 有效性检查
            if (std::isnan(track.targetLongitude) || std::isnan(track.targetLatitude) ||
                std::isnan(track.distance) || track.distance <= 0 ||
                std::isnan(track.azimuth) || std::isnan(track.targetSpeed) ||
                track.targetLongitude < -180 || track.targetLongitude > 180 ||  // 经度范围检查
                track.targetLatitude < -90 || track.targetLatitude > 90) {      // 纬度范围检查
                qWarning() << "Invalid track data received, skipping this track.";
                return;  // 如果数据无效，跳过当前航迹数据
            }

            // 将数据缓存到缓存中
            QJsonObject targetObj;
            targetObj["id"] = track.trackBatchNumber;  // 使用 trackBatchNumber 作为 id
            targetObj["azimuth"] = track.azimuth;
            targetObj["distance"] = track.distance;
            targetObj["speed"] = track.targetSpeed;
            targetObj["latitude"] = track.targetLatitude;
            targetObj["longitude"] = track.targetLongitude;
            targetObj["type"] = track.targetType;

            QJsonObject radarObj;
            radarObj["targets"] = QJsonArray{targetObj};

            QJsonObject rootObj;
            rootObj["radar"] = radarObj;

            // 将解析后的 JSON 数据缓存到 `radarDataCache` 中
            radarDataCache[track.trackBatchNumber] = rootObj;
        }
    }
}

// // 旧数据处理函数
// void MainWindow::onRadarDataParsed(const QByteArray &jsonData)
// {
//     QJsonDocument doc = QJsonDocument::fromJson(jsonData);
//     if (!doc.isObject()) return;

//     QJsonObject radarObj = doc["radar"].toObject();
//     double maxRange = radarObj["maxRange"].toDouble();

//     QJsonArray arr = radarObj["targets"].toArray();
//     for (auto v : arr) {
//         QJsonObject obj = v.toObject();
//         int id = obj["id"].toInt();
//         double angle = obj["azimuth"].toDouble();
//         double distance = obj["distance"].toDouble();
//         double speed = obj["speed"].toDouble();
//         double lat = obj["latitude"].toDouble();
//         double lon = obj["longitude"].toDouble();
//         int type = obj["type"].toInt();

//         // 有效性检查
//         if (std::isnan(angle) || std::isnan(distance) || distance <= 0 ||
//             std::isnan(speed) || std::isnan(lat) || std::isnan(lon)) {
//             qWarning() << "Invalid target data, skipping this target.";
//             continue;  // 如果数据无效，跳过该目标
//         }

//         radar->updateTrack(id, angle, distance / maxRange, speed, lat, lon, type);
//     }
// }

// // 模拟json数据发送
// void MainWindow::simulateJsonData()
// {
//     static std::random_device rd;
//     static std::mt19937 gen(rd());

//     std::uniform_int_distribution<> angleStep(-5, 5);
//     std::uniform_int_distribution<> distStep(-50, 50);
//     std::uniform_int_distribution<> speedStep(-2, 2);
//     std::uniform_int_distribution<> headingStep(-10, 10);
//     std::uniform_int_distribution<> typeDist(1, 3);

//     struct TargetState {
//         double angle;
//         double distance;
//         double speed;
//         double heading;
//         double lat;
//         double lon;
//         int type;
//     };

//     static std::map<int, TargetState> lastStates = {
//         {1, {90, 1000, 10, 90, 30.0000, 120.0000, typeDist(gen)}},
//         {2, {180, 1500, 15, 180, 30.0050, 120.0050, typeDist(gen)}},
//         {3, {270, 800,  20, 270, 30.0100, 120.0100, typeDist(gen)}},
//         {4, {45,  1200, 12, 45,  30.0150, 120.0150, typeDist(gen)}},
//         {5, {135, 600,  18, 135, 30.0200, 120.0200, typeDist(gen)}}
//     };

//     QJsonObject radarObj;
//     radarObj["maxRange"] = 2000;

//     QJsonArray targets;
//     for (int id = 1; id <= 5; id++) {
//         auto &st = lastStates[id];

//         // 更新轨迹
//         st.angle   = std::fmod(st.angle + angleStep(gen) + 360.0, 360.0);
//         st.distance = std::max(200.0, std::min(st.distance + distStep(gen), 2000.0));
//         st.speed   = std::max(0.0, st.speed + speedStep(gen));
//         st.heading = std::fmod(st.heading + headingStep(gen) + 360.0, 360.0);

//         st.lat += 0.0001 * std::cos(st.angle * M_PI / 180.0);
//         st.lon += 0.0001 * std::sin(st.angle * M_PI / 180.0);

//         QJsonObject t;
//         t["id"] = id;
//         t["angle"] = st.angle;
//         t["distance"] = st.distance;
//         t["speed"] = st.speed;
//         t["heading"] = st.heading;
//         t["latitude"] = st.lat;
//         t["longitude"] = st.lon;
//         t["type"] = st.type;
//         targets.append(t);
//     }
//     radarObj["targets"] = targets;

//     QJsonObject root;
//     root["radar"] = radarObj;

//     QJsonDocument doc(root);
//     QByteArray jsonData = doc.toJson();

//     onRadarDataReceived(jsonData);
// }
