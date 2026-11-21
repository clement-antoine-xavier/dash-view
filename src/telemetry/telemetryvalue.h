#pragma once

#include <QMetaType>
#include <QtGlobal>

// One decoded vehicle value. The tag identifies the physical quantity;
// dash-connector is responsible for turning raw CAN into these values.
struct TelemetryValue
{
    quint8 tag = 0;
    double value = 0.0;
};

Q_DECLARE_METATYPE(TelemetryValue)
