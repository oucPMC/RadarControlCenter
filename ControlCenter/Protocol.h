#pragma once
#include <cstdint>
#include <cstring>

#pragma pack(push,1)

// 定义协议帧头 `frame_head_t` ，32B，小端存储
struct FrameHead {
    uint8_t  magic[4];     // "HRGK"
    uint16_t total_len;
    uint16_t device_model;
    uint64_t utc_ms;
    uint16_t msg_id_radar;
    uint16_t msg_id_ext;
    uint16_t device_id_radar;
    uint16_t device_id_ext;
    uint16_t reserved;
    uint8_t  check_mode;
    uint8_t  seq;
    uint32_t msg_count;
};

// 枚举常见报文结构体
enum : uint16_t {
    MSG_ACK       = 0xF000,
    MSG_QUERY     = 0xF001,
    MSG_SILENCE_CFG = 0x2091,
    MSG_SILENCE_FB  = 0x2092,
};

enum : uint8_t { CHECK_NONE=0, CHECK_SUM=1, CHECK_CRC16=2 };

struct MsgQuery {
    FrameHead head;
    uint16_t query_id;
    uint8_t  reserved[16]{};
    uint16_t check;
};

struct MsgAck {
    FrameHead head;
    uint16_t cmd_id;
    int8_t   result;
    uint8_t  reserved[16]{};
    uint16_t check;
};

struct MsgSilenceCfg {
    FrameHead head;
    uint16_t start_001deg;
    uint16_t end_001deg;
    uint8_t  reserved[16]{};
    uint16_t check;
};

// 状态报文结构
struct MsgStatus {
    FrameHead head;
    uint8_t err_hw_sw[4];   // 异常代码
    uint8_t work_state;     // 雷达工作状态
    uint8_t reserved0;
    uint16_t detect_range_m;// 探测范围
    // 仅展示必要部分
    uint8_t reserved[64]{};
    uint16_t check;
};


#pragma pack(pop)

// 提供 `fillFrameHead()` 方法，实现自动填充魔术字、长度、序号自增、时间戳、校验方式。
void fillFrameHead(FrameHead& h, uint16_t msgId, uint16_t totalLen, uint8_t checkMode=2);
