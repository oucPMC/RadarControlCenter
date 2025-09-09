#include <QDataStream>
#ifndef TRACKINFO_H
#define TRACKINFO_H

struct TrackInfo {
    quint16 trackBatchNumber;
    double targetLongitude;
    double targetLatitude;
    float targetAltitude;
    float distance;
    float azimuth;
    float pitch;
    float targetSpeed;
    float targetHeading;
    float targetStrength;
    quint8 reserved1;
    quint8 reserved2;
    quint8 reserved3;
    quint8 reserved4;
    quint8 targetType;
    quint8 targetSize;
    quint8 trackPointType;
    quint8 trackType;
    quint8 consecutiveLostCount;
    quint8 trackQuality;
    float rawDistance;
    float rawAzimuth;
    float rawPitch;
    quint8 reserved5;
    quint8 reserved6;
    quint8 reserved7;
};

#endif // TRACKINFO_H
