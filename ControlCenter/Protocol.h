#pragma once
#include <cstdint>
#include <array>
#include <cstring>

// 1 字节对齐（协议强制 1 字节对齐）
#pragma pack(push,1)

// 协议帧头 "HRGK"（0x48,0x52,0x47,0x4B）
static constexpr uint8_t kMagic[4] = {0x48,0x52,0x47,0x4B};

// 设备型号（默认 6000）
static constexpr uint16_t kDeviceModel = 6000;

// 端口与默认 IP（雷达 192.168.10.10:0x1888，指挥 192.168.20.10:0x1999）
static constexpr uint16_t kRadarPort   = 0x1888;
static constexpr uint16_t kCenterPort  = 0x1999;

// 报文类型标识
enum : uint16_t {
    // 通用
    MSG_ACK                 = 0xF000,   // 指挥中心命令应答报文
    MSG_QUERY               = 0xF001,   // 查询雷达状态/参数报文

    // 指挥中心命令
    MSG_INIT                = 0x1001,
    MSG_CALIB               = 0x1002,
    MSG_STANDBY             = 0x1003,   // 雷达待机任务
    MSG_SEARCH              = 0x1004,   // 雷达搜索任务
    MSG_TRACK               = 0x1005,   // 雷达跟踪任务
    MSG_SIM                 = 0x1006,   // 雷达模拟任务(忽略)
    MSG_POWER               = 0x1007,
    MSG_DEPLOY              = 0x1008,   // 展开/撤收任务
    MSG_SERVO_STOP          = 0x1009,

    // 配置/反馈
    MSG_POS_FB              = 0x2012,
    MSG_ANT_POWER_FB        = 0x2062,
    MSG_SILENCE_CFG         = 0x2091,   // 静默区配置
    MSG_SILENCE_FB          = 0x2092,   // 静默区反馈
    MSG_IP_CFG              = 0x2081,   // 雷达IP配置
    MSG_IP_FB               = 0x2072,   // 雷达IP反馈
    MSG_ERR_COMP_CFG        = 0x2051,   // 误差补偿配置（文档无规定，双方约定即可）

    // 工作数据
    MSG_TRACK_REPORT        = 0x3001,   // 航迹报文（发现即上报）
    MSG_STATUS              = 0x3002,   // 状态报文（50Hz）
};

// 校验方式（0 不校验；1 和校验；2 CRC-16）
enum : uint8_t { CHECK_NONE=0, CHECK_SUM=1, CHECK_CRC16=2 };


// 定义协议帧头，32B，小端存储
struct FrameHead {
    uint8_t  magic[4];       // "HRGK"
    uint16_t total_len;      // 协议数据包总字节数
    uint16_t device_model;   // 6000
    uint64_t utc_ms;         // Unix ms
    uint16_t msg_id_radar;   // 雷达厂家协议ID
    uint16_t msg_id_ext;     // 对端协议ID（一般 0）
    uint16_t device_id_radar;// 组网ID（0 忽略）
    uint16_t device_id_ext;  // 外部设备ID（0 忽略）
    uint16_t reserved;       // 保留
    uint8_t  check_mode;     // 0/1/2
    uint8_t  seq;            // 报文序号 0-255
    uint32_t msg_count;      // 报文计数
};

// 通用查询报文（查询某反馈 ID）
struct MsgQuery {
    FrameHead head;
    uint16_t  query_id;      // 待查询报文ID
    uint8_t   reserved[16]{};
    uint16_t  check;         // 按帧头指定的校验
};

// 命令应答（响应指挥中心命令）
struct MsgAck {
    FrameHead head;
    uint16_t  cmd_id;        // 被响应的指挥中心命令 ID
    int8_t    result;        // 0 收到；>0 成功；<0 失败
    uint8_t   reserved[16]{};
    uint16_t  check;
};


// 任务控制
struct MsgCmdSimple {
    FrameHead head;
    uint8_t reserved[16]{};
    uint16_t check;
};

// 静默区配置（起点/终点，单位 0.01°）
struct MsgSilenceCfg {
    FrameHead head;
    uint16_t  start_001deg;  // 0~36000
    uint16_t  end_001deg;    // 0~36000
    uint8_t   reserved[16]{};
    uint16_t  check;
};


// IP 配置
struct MsgIpConfig {
    FrameHead head;
    uint8_t dsp_mode;      // 0=批量，1=指定
    uint8_t dsp_id;        // DSP号
    uint8_t ip[4];
    uint8_t mask[4];
    uint8_t gateway[4];
    uint16_t port;
    uint8_t reserved[8]{};
    uint16_t check;
};

// 位置/误差补偿
struct MsgPoseComp {
    FrameHead head;
    double latitude;       // 纬度
    double longitude;      // 经度
    float altitude;        // 高度
    float yaw;             // 航向角
    float pitch;           // 俯仰角
    float roll;            // 横滚角
    float sys_az_comp;     // 系统方位补偿
    float sys_el_comp;     // 系统俯仰补偿
    float sys_rng_comp;    // 系统距离补偿
    uint16_t check;
};

// 雷达状态报文
struct MsgStatus {
    FrameHead head;
    uint8_t err_hw_sw[4];   // 异常代码 [0]硬件位;[1:3]软件位
    uint8_t work_state;     // 雷达工作状态
    uint8_t reserved0;
    uint16_t detect_range_m;// 探测范围

    uint8_t   ins_valid;
    uint8_t   sim_on;
    uint8_t   retracted;
    uint8_t   moving;

    double    lon_deg;
    double    lat_deg;
    float     alt_m;

    float     yaw_deg;
    float     pitch_deg;
    float     roll_deg;

    uint8_t   reserved1[8]{};
    uint8_t   ver_reserved[21]{};
    uint8_t   info_reserved[32]{};
    float     freq_GHz;          // 默认 15.80
    uint8_t   ant_power_mode;    // 0~3
    uint8_t   ant_reserved[16]{};
    uint8_t   chan_reserved[8]{};
    uint8_t   servo_reserved[16]{};
    uint16_t  silence_start;
    uint16_t  silence_end;
    uint8_t   reserved2[12]{};
    uint16_t  check;
};

// 航迹信息和报文没写了

// 0x2012 雷达位置反馈：payload = float 经度 + float 纬度（小端）
// 总长 = 32(FrameHead) + 8(payload) + 2(check) = 42 字节
struct MsgPosFeedback {
    FrameHead head;
    float     lon_deg;   // 经度
    float     lat_deg;   // 纬度
    uint16_t  check;
};

// 0x2082 雷达IP反馈：payload = uint32 ip + uint8[8] 预留
struct MsgIpFeedback {
    FrameHead head;
    uint32_t  ip;           // 小端 32-bit
    uint8_t   reserved[8]{};
    uint16_t  check;
};

// 0x2062 天线上电模式反馈：payload = uint8 模式 + uint8[11] 预留
struct MsgAntPowerFeedback {
    FrameHead head;
    uint8_t   ant_power_mode;   // 0~?
    uint8_t   reserved[11]{};
    uint16_t  check;
};
#pragma pack(pop)

// 提供 `fillFrameHead()` 方法，实现自动填充协议帧头、长度、序号自增、时间戳、校验方式。
void fillFrameHead(FrameHead& h, uint16_t msgId, uint16_t totalLen, uint8_t checkMode=2);
