#### **`Protocol.h/.cpp`** ：

统一报文格式以及报文填充

- 定义协议帧头 `FrameHead`
- 定义常见报文结构体：
  - 查询报文
  - 应答报文
  - 任务控制
  - 静默区配置报文
  - IP 配置报文
  - 位置/误差补偿报文
  - 雷达状态报文
- 提供 `fillFrameHead()` 方法：自动填充协议帧头、长度、序号自增、时间戳、校验方式。



#### **`PacketCodec.h/.cpp`**：

提供校验函数和验证函数，若校验字段为1则使用和检验，为2则使用CRC16



#### **`UdpLink.h/.cpp`**：

实现udp网络的封装，用于方便地发送和接收udp数据报，并通过信号与槽机制通知外部接收到的数据。

- 封装 `QUdpSocket`，支持单播/广播/本机 loopback。

- 提供接口：

  ```cpp
  // 绑定本地地址和端口
  bool bind(const QHostAddress& addr, quint16 port);
  // 设置目标地址和端口
  void setPeer(const QHostAddress& host, quint16 port);
  // 向目标地址发送数据dat
  qint64 send(const QByteArray& dat);
  signals:
  	// 收到udp数据包时发出信号
      void packetReceived(QByteArray dat, QHostAddress from, quint16 port);
  private slots:
      // 槽函数，当内部的udp套接字对象 sock_ 有数据可读时自动调用
      void onReadyRead();
  ```



#### **`CommManager.h/.cpp`**：

实现核心的报文通信控制器

1. 基础通信管理（封装了底层 `UdpLink`，负责 UDP 收发）

   - **bind()**：绑定本机监听地址和端口（指挥中心192.168.20.10: 0x1999）。

   - **setPeer()**：设置雷达对端的 IP 和端口（雷达 192.168.10.10:0x1888）。

2. 任务控制报文（提供了协议定义的四个雷达控制接口，这些函数内部会构造对应报文，调用 `sendWithAck()`下发命令并等待 ACK）

   - `sendStandby()` → 0x1003：切换到待机模式

   - `sendSearch()` → 0x1004：切换到搜索模式

   - `sendTrack()` → 0x1005：切换到跟踪模式

   - `sendDeploy()` → 0x1008：执行展开/撤收

3. 参数配置报文（实现了主要的配置下发功能：静默区、IP、补偿）

   -  `sendSilentZone(start, end)` → 0x2091：设置雷达扫描屏蔽范围（起点/终点角度）。
   -  `sendIpConfig(ip, mask, gw, port, dspMode, dspId)` → 0x2081：下发新 IP/掩码/网关/端口配置，以广播方式下发，雷达重启后生效

   -  `sendPoseComp(lat, lon, alt, yaw, pitch, roll, azComp, elComp, rngComp)` → 0x2011：下发经纬度、姿态角和系统补偿参数。

4. 查询机制
   - `query(queryId)`：查询雷达参数， 使用 `0xF001` 查询报文，雷达会返回对应的反馈报文。

5. UI 交互接口（信号槽）

   - `ackReceived(cmdId, result)`：收到某个命令的执行结果

   - `commError(cmdId)`：某个命令超时/重发次数超限

   - `logMessage(msg)`：日志消息（可以 append 到 QTextEdit）

   - `statusReceived(workState, range)`：收到状态报文，实时更新，UI 可以显示雷达工作状态和探测范围。

6. 报文收发 & 解析（内部槽函数`onPkt`）

   - 报文合法性校验：调用 `verifyPacket()`，检查 magic、长度、校验和。

   - 检查报文类型：

     - ACK 报文 (0xF000)：匹配报文序号 `seq`，找到对应命令；停止超时定时器，发 `ackReceived(cmdId, result)` 信号；示例性触发查询闭环（自动发一次参数查询）

     - 状态报文 (0x3002)：解析 `work_state`（雷达当前工作模式），解析 `detect_range_m`（当前探测范围），发 `statusReceived(workState, range)` 信号，供 UI 实时更新状态栏

7. ACK 超时 & 重传机制（`onTimeout()` 和 `sendWithAck()`）

   - 每个命令下发时会建立 `PendingCmd`，存入 `pending_`，启动 1 秒定时器。

   - 超时未收到 ACK → 自动重发，最多 3 次。

   - 超过 3 次仍失败 → 发 `commError(cmdId)` 信号。