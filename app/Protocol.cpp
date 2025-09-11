#include "Protocol.h"
#include <chrono>

static uint8_t g_seq = 0;

static inline uint64_t nowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

void fillFrameHead(FrameHead& h, uint16_t msg_id, uint16_t total_len, uint8_t check_mode) {
    std::memcpy(h.magic, kMagic, 4);
    h.total_len     = total_len;          // 小端主机，协议默认低字节在前 :contentReference[oaicite:28]{index=28}
    h.device_model  = kDeviceModel;
    h.utc_ms        = nowMs();
    h.msg_id_radar  = msg_id;
    h.msg_id_ext    = 0;
    h.device_id_radar = 0;
    h.device_id_ext = 0;
    h.reserved      = 0;
    h.check_mode    = check_mode;         // 1 和校验；2 CRC16
    h.seq           = g_seq++;
    h.msg_count++;
}
