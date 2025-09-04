#include "PacketCodec.h"

uint16_t crc16_modbus(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i=0;i<len;++i) {
        crc ^= data[i];
        for (int j=0;j<8;j++) {
            if (crc & 1) crc = (crc >> 1) ^ 0xA001;
            else crc >>= 1;
        }
    }
    return crc;
}

uint16_t sum16(const uint8_t* data, size_t len) {
    uint32_t sum=0;
    for(size_t i=0;i<len;++i) sum+=data[i];
    return uint16_t(sum & 0xFFFF);
}

bool verifyPacket(const QByteArray& dat) {
    if (dat.size() < int(sizeof(FrameHead)+2)) return false;
    const FrameHead* h = reinterpret_cast<const FrameHead*>(dat.data());
    if (memcmp(h->magic, "HRGK", 4)!=0) return false;
    if (h->total_len != dat.size()) return false;
    size_t withoutTail = dat.size()-2;
    uint16_t calc=0;
    if (h->check_mode==CHECK_CRC16) calc=crc16_modbus((const uint8_t*)dat.data(), withoutTail);
    else if (h->check_mode==CHECK_SUM) calc=sum16((const uint8_t*)dat.data(), withoutTail);
    uint16_t given=*reinterpret_cast<const uint16_t*>(dat.data()+withoutTail);
    return (calc==given);
}
