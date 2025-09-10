QT += core gui widgets network

SOURCES += \
    CommManager.cpp \
    PacketCodec.cpp \
    Protocol.cpp \
    UdpLink.cpp \
    main.cpp \
    Configure.cpp

HEADERS += \
    CommManager.h \
    Configure.h \
    Protocol.h \
    PacketCodec.h \
    UdpLink.h

FORMS += \
    Configure.ui

RESOURCES += \
    Configure.qrc
