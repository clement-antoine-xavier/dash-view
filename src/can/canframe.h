#pragma once

#include <QMetaType>
#include <QtGlobal>

#include <array>

// A classical (11-bit) CAN frame with up to 8 data bytes.
struct CanFrame
{
    quint32 id = 0;
    quint8 dlc = 0;
    std::array<quint8, 8> data{};
    qint64 timestampMs = 0; // monotonic arrival time, milliseconds
};

Q_DECLARE_METATYPE(CanFrame)
