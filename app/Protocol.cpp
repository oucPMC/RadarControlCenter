#include "Protocol.h"
#include <chrono>

static uint8_t g_seq = 0;
static uint32_t g_msgCount = 0;

uint64_t nowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

void fillFrameHead(FrameHead& h, uint16_t msgId, uint16_t totalLen, uint8_t checkMode)
{
    const uint8_t magic[4] = {0x48,0x52,0x47,0x4B};
    memcpy(h.magic, magic, 4);
    h.total_len = totalLen;
    h.device_model = 6000;
    h.utc_ms = nowMs();
    h.msg_id_radar = msgId;
    h.msg_id_ext = 0;
    h.device_id_radar = 0;
    h.device_id_ext = 0;
    h.reserved = 0;
    h.check_mode = checkMode;
    h.seq = g_seq++;
    h.msg_count = g_msgCount++;
}
