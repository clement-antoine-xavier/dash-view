#pragma once

#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QBluetoothUuid>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QTimer>

#include "telemetry/telemetrysource.h"

// BLE central for the Arduino tagged-union stream. Experimental: the firmware
// side is a separate project and is not included in this repository.
class BleTelemetrySource final : public TelemetrySource
{
    Q_OBJECT

public:
    explicit BleTelemetrySource(QString nameFilter = {}, QObject *parent = nullptr);
    ~BleTelemetrySource() override;

    void start() override;
    void stop() override;
    QString name() const override;
    QString description() const override;
    QString label() const override;

private slots:
    void onDeviceDiscovered(const QBluetoothDeviceInfo &info);
    void onScanFinished();
    void onControllerConnected();
    void onControllerDisconnected();
    void onControllerError(QLowEnergyController::Error error);
    void onServiceDiscovered(const QBluetoothUuid &uuid);
    void onServiceDiscoveryFinished();
    void onServiceStateChanged(QLowEnergyService::ServiceState state);
    void onCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);
    void onCharacteristicRead(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);
    void onServiceError(QLowEnergyService::ServiceError error);
    void reconnect();

private:
    void startScan();
    void cleanupConnection();
    void fail(const QString &message);
    static bool matches(const QBluetoothDeviceInfo &info, const QString &filter);

    QString m_nameFilter;
    QString m_deviceName;
    QBluetoothDeviceDiscoveryAgent *m_discovery = nullptr;
    QLowEnergyController *m_controller = nullptr;
    QLowEnergyService *m_service = nullptr;
    QTimer m_reconnectTimer;
    bool m_stopped = true;
    bool m_scanning = false;
    bool m_connecting = false;
    bool m_foundService = false;
};
