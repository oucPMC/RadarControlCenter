#include "mainwindow.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <random>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *layout = new QVBoxLayout(central);

    radar = new RadarPlot(this);
    layout->addWidget(radar);

    startButton = new QPushButton("Start Scan", this);
    stopButton  = new QPushButton("Stop Scan", this);
    layout->addWidget(startButton);
    layout->addWidget(stopButton);

    connect(startButton, &QPushButton::clicked, this, &MainWindow::startSimulation);
    connect(stopButton,  &QPushButton::clicked, this, &MainWindow::stopSimulation);

    simTimer = new QTimer(this);
    connect(simTimer, &QTimer::timeout, this, &MainWindow::simulateJsonData);
}

void MainWindow::startSimulation()
{
    radar->startScan();
    simTimer->start(1000);
}

void MainWindow::stopSimulation()
{
    radar->stopScan();
    simTimer->stop();
}

void MainWindow::onRadarDataReceived(const QByteArray &jsonData)
{
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (!doc.isObject()) return;

    QJsonObject radarObj = doc["radar"].toObject();
    double maxRange = radarObj["maxRange"].toDouble();

    QJsonArray arr = radarObj["targets"].toArray();
    for (auto v : arr) {
        QJsonObject obj = v.toObject();
        int id = obj["id"].toInt();
        double angle = obj["angle"].toDouble();
        double distance = obj["distance"].toDouble();
        double speed = obj["speed"].toDouble();
        double lat = obj["latitude"].toDouble();
        double lon = obj["longitude"].toDouble();
        int type = obj["type"].toInt();

        radar->updateTrack(id, angle, distance / maxRange, speed, lat, lon, type);
    }
}

void MainWindow::simulateJsonData()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::uniform_int_distribution<> angleStep(-5, 5);
    std::uniform_int_distribution<> distStep(-50, 50);
    std::uniform_int_distribution<> speedStep(-2, 2);
    std::uniform_int_distribution<> headingStep(-10, 10);
    std::uniform_int_distribution<> typeDist(1, 3);

    struct TargetState {
        double angle;
        double distance;
        double speed;
        double heading;
        double lat;
        double lon;
        int type;   // ← 加上 type
    };

    static std::map<int, TargetState> lastStates = {
        {1, {90, 1000, 10, 90, 30.0000, 120.0000, typeDist(gen)}},
        {2, {180, 1500, 15, 180, 30.0050, 120.0050, typeDist(gen)}},
        {3, {270, 800,  20, 270, 30.0100, 120.0100, typeDist(gen)}},
        {4, {45,  1200, 12, 45,  30.0150, 120.0150, typeDist(gen)}},
        {5, {135, 600,  18, 135, 30.0200, 120.0200, typeDist(gen)}}
    };

    QJsonObject radarObj;
    radarObj["maxRange"] = 2000;

    QJsonArray targets;
    for (int id = 1; id <= 5; id++) {
        auto &st = lastStates[id];

        // 更新轨迹
        st.angle   = std::fmod(st.angle + angleStep(gen) + 360.0, 360.0);
        st.distance = std::clamp(st.distance + distStep(gen), 200.0, 2000.0);
        st.speed   = std::max(0.0, st.speed + speedStep(gen));
        st.heading = std::fmod(st.heading + headingStep(gen) + 360.0, 360.0);

        st.lat += 0.0001 * std::cos(st.angle * M_PI / 180.0);
        st.lon += 0.0001 * std::sin(st.angle * M_PI / 180.0);

        QJsonObject t;
        t["id"] = id;
        t["angle"] = st.angle;
        t["distance"] = st.distance;
        t["speed"] = st.speed;
        t["heading"] = st.heading;
        t["latitude"] = st.lat;
        t["longitude"] = st.lon;
        t["type"] = st.type;   // ← 用固定的 type
        targets.append(t);
    }
    radarObj["targets"] = targets;

    QJsonObject root;
    root["radar"] = radarObj;

    QJsonDocument doc(root);
    QByteArray jsonData = doc.toJson();

    onRadarDataReceived(jsonData);
}

