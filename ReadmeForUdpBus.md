# UdpBus 

## 1. 模块整体结构

本项目的通信分为三层：

```
UI 层 (MainWindow)
       │
       ▼
抽象接口 (BusIface)
       │
       ▼
实现层 (UdpBus)
       │
       ▼
协议层 (Protocol + PacketCodec)
       │
       ▼
网络层 (QUdpSocket / UdpLink)
```

- **UI 层**：界面逻辑，调用 `sendCommand()` 等接口，监听信号 `busAck() / bus3002()` 等。
- **BusIface**：抽象接口，定义标准的通信 API，便于未来替换底层实现（如 TCP/串口）。
- **UdpBus**：具体实现，负责收发 UDP 报文，并调用协议层提供的工具。
- **Protocol**：协议层，定义报文结构体、帧头、辅助函数（`fillFrameHead()`）。
- **PacketCodec**：报文序列化和校验工具，提供 `encode()`、`verifyPacket()`、`crc16_modbus()` 等。
- **QUdpSocket**：Qt 自带的 UDP 网络接口，用于真正的数据发送和接收。

------

## 2. 核心类接口

### 2.1 BusIface（抽象接口）

主要方法：

- `setTargets(QList<Target>)`：设置目标 IP + 端口（可多个）。
- `start(localPort, Policy)`：绑定本机端口，启动通信。
- `stop()`：停止通信，释放资源。
- `sendCommand(msgId, payload)`：统一命令发送入口，返回序号 `seq`。
- `subscribe(msgId, enable)`：订阅/退订报文。

主要信号：

- `busTx(msgId, seq, targets, note)`：命令下发日志。
- `busAck(ackId, respondedId, seq, result, note)`：收到 ACK 时触发。
- `busPayload(msgId, seq, payload)`：收到一般负载报文时触发。
- `bus3002(fields)`：收到 0x3002 状态报文时触发。

------

### 2.2 UdpBus（实现类）

继承自 BusIface，完成具体逻辑。

- **启动/停止**

  ```
  bool start(quint16 localPort, const Policy& policy);
  void stop();
  ```

  - 使用 `QUdpSocket::bind()` 绑定本机端口。
  - 配置策略（超时、重传次数）。

- **目标配置**

  ```
  void setTargets(const QList<Target>& targets);
  ```

  - 保存需要下发的目标（可能是多个雷达）。

- **命令发送**

  ```
  quint16 sendCommand(quint16 msgId, const QByteArray& payload);
  ```

  - 根据 `msgId` 构造对应的协议报文（Protocol 提供结构体）。
  - 调用 `fillFrameHead()` 填充帧头（长度、序号、时间戳、校验方式）。
  - 用 `PacketCodec::encode()` 序列化成 `QByteArray`。
  - 计算校验（`crc16_modbus` 或 `sum16`），写入报文尾部。
  - 通过 `QUdpSocket::writeDatagram()` 广播给目标。
  - 发射 `busTx()` 信号。

- **订阅机制**

  ```
  void subscribe(quint16 msgId, bool enable);
  ```

  - 内部维护一个 `QSet<quint16>`，只转发订阅过的报文到 UI。

- **接收处理**

  ```
  void onReadyRead();
  ```

  - 读取 UDP 报文。
  - 调用 `verifyPacket()` 校验报文合法性。
  - 提取 `FrameHead`，根据 `msgId` 分发：
    - `0xF000 (ACK)` → 转换为 `MsgAck`，发射 `busAck()`。
    - `0x3002 (状态)` → 转换为 `MsgStatus`，填入 `QVariantMap`，发射 `bus3002()`。
    - 其他订阅报文 → 截取 payload，发射 `busPayload()`。

------

### 2.3 Protocol（协议层）

- **FrameHead**：定义通用帧头结构（magic、长度、序号、时间戳、校验模式）。
- **报文结构体**：
  - `MsgCmdSimple`（待机、搜索、跟踪、展开）。
  - `MsgSilenceCfg`（静默区配置）。
  - `MsgIpConfig`（网络参数配置）。
  - `MsgPoseComp`（姿态/补偿参数）。
  - `MsgStatus`（状态报文 0x3002）。
- **辅助函数**：
  - `fillFrameHead()`：自动填充帧头。

------

### 2.4 PacketCodec（编码/校验工具）

- `QByteArray encode<T>(const T& pkt)`：结构体序列化。
- `bool verifyPacket(const QByteArray& dat)`：验证报文合法性。
- `uint16_t crc16_modbus(const uint8_t*, size_t)`：CRC16 校验。
- `uint16_t sum16(const uint8_t*, size_t)`：和检验。

------

## 3. 调用流程示例

### 3.1 发送命令

UI 调用：

```
bus->sendCommand(MSG_STANDBY, QByteArray());
```

内部流程：

1. `UdpBus::sendCommand()` → 构造 `MsgCmdSimple`。
2. `fillFrameHead()` → 自动写入帧头（长度、序号、校验方式）。
3. `encode(pkt)` → 转成 QByteArray。
4. `crc16_modbus()` → 填写报文尾部校验。
5. `QUdpSocket::writeDatagram()` → 发给目标。
6. 发射 `busTx()` 信号，UI 记录日志。

------

### 3.2 接收报文

雷达回传 UDP 报文 → `QUdpSocket::readyRead()` → `UdpBus::onReadyRead()`：

1. `verifyPacket(dat)` → 检查合法性。
2. 判断 `msgId`：
   - **ACK (0xF000)** → 转成 `MsgAck`，发射 `busAck()`。
   - **状态 (0x3002)** → 转成 `MsgStatus`，整理为 `QVariantMap`，发射 `bus3002()`。
   - **其他订阅报文** → 截取 payload，发射 `busPayload()`。

UI 侧即可：

```
connect(bus, &UdpBus::bus3002, this, &MainWindow::onStatusUpdate);
```

------

## 4. 信号和 UI 显示

- **busTx** → UI 记录下行日志（命令下发）。
- **busAck** → UI 显示命令执行结果（成功/失败）。
- **busPayload** → UI 显示查询结果或其他数据。
- **bus3002** → UI 实时更新雷达状态（模式、范围、频率等）。

------

## 5. 总结

- **Protocol** 负责定义报文格式。

- **PacketCodec** 负责报文编解码和校验。

- **UdpBus** 把二者结合起来，实现完整的收发逻辑。

- **UI 只需要关心 BusIface 的接口和信号，不必关心底层协议细节**。

  

调用时序图（发送 + 接收）

```
┌───────────┐         ┌──────────┐        ┌────────────┐         ┌───────────┐
│   UI层    │         │  UdpBus  │        │  Protocol  │         │ PacketCodec│
│(MainWindow│         │          │        │ (结构体定义)│         │ (编解码+校验)│
└─────┬─────┘         └────┬─────┘        └─────┬──────┘         └────┬───────┘
      │  sendCommand(MSG_STANDBY) │              │                     │
      │──────────────────────────>│              │                     │
      │                           │ fillFrameHead()                    │
      │                           │──────────────>│                    │
      │                           │              │ 返回完整FrameHead    │
      │                           │<─────────────│                     │
      │                           │ encode(pkt)                        │
      │                           │───────────────────────────────────>│
      │                           │                                    │
      │                           │          生成 QByteArray            │
      │                           │<───────────────────────────────────│
      │                           │ crc16_modbus()/sum16()             │
      │                           │───────────────────────────────────>│
      │                           │                                    │
      │                           │ 校验值写入报文尾部                    │
      │                           │<───────────────────────────────────│
      │                           │ writeDatagram() → UDP 网络          │
      │                           │──────────────> [ 雷达设备 ]         │
      │                           │                                    │
      │        emit busTx()       │                                    │
      │<──────────────────────────│                                    │

```



接收流程（雷达回复报文）

```
 [雷达设备]
      │ UDP 报文
      │──────────────────────────>│
┌─────┴─────┐         ┌──────────┐        ┌────────────┐         ┌───────────┐
│   UI层    │         │  UdpBus  │        │  Protocol  │         │ PacketCodec│
│(MainWindow│         │          │        │ (结构体定义)│         │ (编解码+校验)│
└─────┬─────┘         └────┬─────┘        └─────┬──────┘         └────┬───────┘
      │                           │ onReadyRead()                     │
      │                           │ verifyPacket(dat)                  │
      │                           │───────────────────────────────────>│
      │                           │                                    │
      │                           │    校验合法性 (magic,len,crc)      │
      │                           │<───────────────────────────────────│
      │                           │ 解析 FrameHead.msgId               │
      │                           │                                    │
      │  busAck() (ACK)           │                                    │
      │<──────────────────────────│                                    │
      │                           │ 或 bus3002(fields) (状态报文)       │
      │<──────────────────────────│                                    │
      │                           │ 或 busPayload() (其他订阅报文)      │
      │<──────────────────────────│                                    │

```

