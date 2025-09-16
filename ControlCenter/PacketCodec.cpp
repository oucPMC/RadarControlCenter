#include "PacketCodec.h"

// 实现标准的 CRC-16/MODBUS 算法，用于雷达协议报文的可靠性校验。
uint16_t crc16_modbus(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;             // 初始值 0xFFFF
    for (size_t i=0;i<len;++i) {
        crc ^= data[i];                // 将当前字节与 CRC 低字节异或
        for (int j=0;j<8;j++) {        // 遍历每一位
            if (crc & 1)               // 如果最低位是 1
                crc = (crc >> 1) ^ 0xA001; // 右移一位并与多项式 0xA001 异或
            else
                crc >>= 1;             // 否则直接右移一位
        }
    }
    return crc;                        // 返回最终的 16 位 CRC 校验值
}


// 简单的字节累加和，取低 16 位作为校验结果，check_mode = 1 (和校验) 时使用
uint16_t sum16(const uint8_t* data, size_t len) {
    uint32_t sum=0;
    for(size_t i=0;i<len;++i) sum+=data[i]; // 累加所有字节
    return uint16_t(sum & 0xFFFF); // 截取低 16 位作为结果
}

// 验证一帧雷达报文是否有效
// 1. 报文长度是否够
// 2. 魔术字是否正确 ("HRGK")
// 3. 长度字段是否匹配
// 4. 校验值是否一致（根据 check_mode 选择 CRC16 或和校验）
bool verifyPacket(const QByteArray& dat) {
    // 报文长度小于帧头+校验字段 → 无效报文
    if (dat.size() < int(sizeof(FrameHead)+2)) return false;

    // 检查帧头 magic 字段是否等于 "HRGK"
    const FrameHead* h = reinterpret_cast<const FrameHead*>(dat.data());
    if (memcmp(h->magic, "HRGK", 4)!=0) return false;

    // 报文声明长度 != 实际长度 → 无效报文
    if (h->total_len != dat.size()) return false;

    size_t withoutTail = dat.size()-2; // 去掉最后两个字节（校验值）
    uint16_t calc=0;
    // 根据帧头指定的 check_mode 选择 CRC16 或和校验计算结果
    if (h->check_mode==CHECK_CRC16)
        calc=crc16_modbus((const uint8_t*)dat.data(), withoutTail);
    else if (h->check_mode==CHECK_SUM)
        calc=sum16((const uint8_t*)dat.data(), withoutTail);

    // 取报文末尾的 2 字节校验值
    uint16_t given=*reinterpret_cast<const uint16_t*>(dat.data()+withoutTail);

    // 返回 true/false，判断计算结果与报文携带的校验值是否一致
    return (calc==given);

    //     // 报文长度小于帧头+校验字段 → 无效报文
    // if (dat.size() < int(sizeof(FrameHead)+2)) {
    //     std::cout<<"Packet too short: "<<dat.size()<<endl;
    //     return false;
    // }

    // // 检查帧头 magic 字段是否等于 "HRGK"
    // const FrameHead* h = reinterpret_cast<const FrameHead*>(dat.data());
    // if (memcmp(h->magic, "HRGK", 4)!=0) {
    //     cout<<"Invalid magic: "<<QString::fromUtf8(h->magic, 4)<<endl;
    //     return false;
    // }

    // // 报文声明长度 != 实际长度 → 无效报文
    // if (h->total_len != dat.size()) {
    //     cout<<"Length mismatch: header "<<h->total_len<<", actual "<<dat.size()<<endl;
    //     return false;
    // }

    // size_t withoutTail = dat.size()-2; // 去掉最后两个字节（校验值）
    // uint16_t calc=0;
    // // 根据帧头指定的 check_mode 选择 CRC16 或和校验计算结果
    // if (h->check_mode==CHECK_CRC16)
    //     calc=crc16_modbus((const uint8_t*)dat.data(), withoutTail);
    // else if (h->check_mode==CHECK_SUM)
    //     calc=sum16((const uint8_t*)dat.data(), withoutTail);

    // // 取报文末尾的 2 字节校验值
    // uint16_t given=*reinterpret_cast<const uint16_t*>(dat.data()+withoutTail);

    // // 返回 true/false，判断计算结果与报文携带的校验值是否一致
    // return (calc==given);
}
