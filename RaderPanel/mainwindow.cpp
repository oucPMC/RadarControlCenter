#include "mainwindow.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <random>
#include <QStyleFactory>
#include <QPalette>
#include <QApplication>

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
        border: 1px solid #00ffaa;
        border-radius: 6px;
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
    resize(800, 700);
    setWindowTitle("雷达控制系统");

    central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *layout = new QVBoxLayout(central);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

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

    startButton->setMinimumSize(120, 40);
    stopButton->setMinimumSize(120, 40);

    buttonLayout->addWidget(startButton);
    buttonLayout->addWidget(stopButton);
    buttonLayout->setAlignment(Qt::AlignCenter);
    buttonLayout->setSpacing(20);

    layout->addLayout(buttonLayout);

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
        int type;
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
        st.distance = std::max(200.0, std::min(st.distance + distStep(gen), 2000.0));
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
        t["type"] = st.type;
        targets.append(t);
    }
    radarObj["targets"] = targets;

    QJsonObject root;
    root["radar"] = radarObj;

    QJsonDocument doc(root);
    QByteArray jsonData = doc.toJson();

    onRadarDataReceived(jsonData);
}
