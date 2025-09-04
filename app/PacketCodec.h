#pragma once
#include "Protocol.h"
#include <QByteArray>

uint16_t crc16_modbus(const uint8_t* data, size_t len);
uint16_t sum16(const uint8_t* data, size_t len);

template<typename T>
QByteArray encode(const T& pkt) {
    return QByteArray(reinterpret_cast<const char*>(&pkt), sizeof(T));
}

bool verifyPacket(const QByteArray& dat);
