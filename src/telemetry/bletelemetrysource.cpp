#include "telemetry/bletelemetrysource.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

#include "telemetry/telemetrycodec.h"

namespace {

constexpr char kServiceUuid[] = "6b8d0001-1f2e-4a3b-9c4d-5e6f7a8b9c0d";
constexpr char kNotifyUuid[] = "6b8d0002-1f2e-4a3b-9c4d-5e6f7a8b9c0d";
constexpr char kInfoUuid[] = "6b8d0003-1f2e-4a3b-9c4d-5e6f7a8b9c0d";
constexpr int kReconnectMs = 3000;

QBluetoothUuid telemetryService()
{
    return QBluetoothUuid(QString::fromLatin1(kServiceUuid));
}

QBluetoothUuid telemetryNotify()
{
    return QBluetoothUuid(QString::fromLatin1(kNotifyUuid));
}

QBluetoothUuid telemetryInfo()
{
    return QBluetoothUuid(QString::fromLatin1(kInfoUuid));
}

} // namespace

BleTelemetrySource::BleTelemetrySource(QString nameFilter, QObject *parent)
    : TelemetrySource(parent)
    , m_nameFilter(std::move(nameFilter))
{
    m_discovery = new QBluetoothDeviceDiscoveryAgent(this);
    m_discovery->setLowEnergyDiscoveryTimeout(8000);

    connect(m_discovery, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &BleTelemetrySource::onDeviceDiscovered);
    connect(m_discovery, &QBluetoothDeviceDiscoveryAgent::finished,
            this, &BleTelemetrySource::onScanFinished);
    connect(m_discovery, &QBluetoothDeviceDiscoveryAgent::errorOccurred,
            this, [this](QBluetoothDeviceDiscoveryAgent::Error) {
                if (m_stopped)
                    return;
                fail(QStringLiteral("Bluetooth scan failed: %1").arg(m_discovery->errorString()));
            });

    m_reconnectTimer.setSingleShot(true);
    m_reconnectTimer.setInterval(kReconnectMs);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &BleTelemetrySource::reconnect);
}

BleTelemetrySource::~BleTelemetrySource()
{
    stop();
}

void BleTelemetrySource::start()
{
    if (!m_stopped)
        return;

    m_stopped = false;
    cleanupConnection();
    startScan();
}

void BleTelemetrySource::stop()
{
    m_stopped = true;
    m_reconnectTimer.stop();
    cleanupConnection();
    setState(State::Stopped, QStringLiteral("Stopped"));
}

QString BleTelemetrySource::name() const
{
    return QStringLiteral("ble");
}

QString BleTelemetrySource::description() const
{
    if (m_deviceName.isEmpty())
        return QStringLiteral("BLE telemetry");
    return QStringLiteral("BLE · ") + m_deviceName;
}

QString BleTelemetrySource::label() const
{
    return QStringLiteral("BLE");
}

void BleTelemetrySource::startScan()
{
    if (m_stopped)
        return;

    m_connecting = false;
    m_foundService = false;
    m_scanning = true;
    setState(State::Starting, m_deviceName.isEmpty()
                                 ? QStringLiteral("Scanning for BLE device…")
                                 : QStringLiteral("Reconnecting to %1…").arg(m_deviceName));
    m_discovery->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}

void BleTelemetrySource::onDeviceDiscovered(const QBluetoothDeviceInfo &info)
{
    if (m_stopped || !m_scanning || m_connecting)
        return;

    if (!(info.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration))
        return;

    if (!matches(info, m_nameFilter))
        return;

    m_scanning = false;
    m_connecting = true;
    m_discovery->stop();

    m_deviceName = info.name().isEmpty() ? QStringLiteral("BLE vehicle") : info.name();
    setVehicleName(m_deviceName);
    setState(State::Starting, QStringLiteral("Connecting to %1…").arg(m_deviceName));

    m_controller = QLowEnergyController::createCentral(info, this);
    connect(m_controller, &QLowEnergyController::connected,
            this, &BleTelemetrySource::onControllerConnected);
    connect(m_controller, &QLowEnergyController::disconnected,
            this, &BleTelemetrySource::onControllerDisconnected);
    connect(m_controller, &QLowEnergyController::errorOccurred,
            this, &BleTelemetrySource::onControllerError);
    connect(m_controller, &QLowEnergyController::serviceDiscovered,
            this, &BleTelemetrySource::onServiceDiscovered);
    connect(m_controller, &QLowEnergyController::discoveryFinished,
            this, &BleTelemetrySource::onServiceDiscoveryFinished);

    m_controller->connectToDevice();
}

void BleTelemetrySource::onScanFinished()
{
    if (!m_scanning)
        return;

    m_scanning = false;

    if (m_stopped || m_connecting || m_controller)
        return;

    setState(State::Error, QStringLiteral("No matching BLE device found"));
    m_reconnectTimer.start();
}

void BleTelemetrySource::onControllerConnected()
{
    if (m_stopped || !m_controller)
        return;

    setState(State::Starting, QStringLiteral("Discovering telemetry service…"));
    m_controller->discoverServices();
}

void BleTelemetrySource::onControllerDisconnected()
{
    if (m_stopped)
        return;

    setState(State::Starting, QStringLiteral("Disconnected, retrying…"));
    cleanupConnection();
    m_reconnectTimer.start();
}

void BleTelemetrySource::onControllerError(QLowEnergyController::Error error)
{
    if (m_stopped)
        return;

    Q_UNUSED(error);
    fail(QStringLiteral("BLE controller error: %1").arg(m_controller ? m_controller->errorString()
                                                                   : QStringLiteral("unknown")));
}

void BleTelemetrySource::onServiceDiscovered(const QBluetoothUuid &uuid)
{
    if (uuid == telemetryService())
        m_foundService = true;
}

void BleTelemetrySource::onServiceDiscoveryFinished()
{
    if (m_stopped || !m_controller)
        return;

    if (!m_foundService) {
        fail(QStringLiteral("Telemetry service not found"));
        return;
    }

    m_service = m_controller->createServiceObject(telemetryService(), this);
    if (!m_service) {
        fail(QStringLiteral("Could not open telemetry service"));
        return;
    }

    connect(m_service, &QLowEnergyService::stateChanged,
            this, &BleTelemetrySource::onServiceStateChanged);
    connect(m_service, &QLowEnergyService::characteristicChanged,
            this, &BleTelemetrySource::onCharacteristicChanged);
    connect(m_service, &QLowEnergyService::characteristicRead,
            this, &BleTelemetrySource::onCharacteristicRead);
    connect(m_service, &QLowEnergyService::errorOccurred,
            this, &BleTelemetrySource::onServiceError);

    m_service->discoverDetails();
}

void BleTelemetrySource::onServiceStateChanged(QLowEnergyService::ServiceState state)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    const auto discovered = QLowEnergyService::RemoteServiceDiscovered;
#else
    const auto discovered = QLowEnergyService::ServiceDiscovered;
#endif

    if (state != discovered || !m_service)
        return;

    const QLowEnergyCharacteristic notify = m_service->characteristic(telemetryNotify());
    if (!notify.isValid() || !(notify.properties() & QLowEnergyCharacteristic::Notify)) {
        fail(QStringLiteral("Notify characteristic missing"));
        return;
    }

    const QLowEnergyDescriptor cccd = notify.descriptor(QBluetoothUuid(quint16(0x2902)));
    if (!cccd.isValid()) {
        fail(QStringLiteral("Notification descriptor missing"));
        return;
    }

    m_service->writeDescriptor(cccd, QByteArray::fromHex("0100"));

    const QLowEnergyCharacteristic info = m_service->characteristic(telemetryInfo());
    if (info.isValid() && (info.properties() & QLowEnergyCharacteristic::Read))
        m_service->readCharacteristic(info);

    setState(State::Running, QStringLiteral("Connected to %1").arg(m_deviceName));
}

void BleTelemetrySource::onCharacteristicChanged(const QLowEnergyCharacteristic &characteristic,
                                                 const QByteArray &value)
{
    if (characteristic.uuid() != telemetryNotify())
        return;

    const QVector<TelemetryValue> values = telemetry::decodeNotification(value);
    for (const TelemetryValue &telemetryValue : values)
        emit valueReceived(telemetryValue);
}

void BleTelemetrySource::onCharacteristicRead(const QLowEnergyCharacteristic &characteristic,
                                              const QByteArray &value)
{
    if (characteristic.uuid() != telemetryInfo())
        return;

    const QByteArray trimmed = value.trimmed();
    if (trimmed.isEmpty())
        return;

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(trimmed, &error);
    if (error.error == QJsonParseError::NoError && document.isObject()) {
        const QString name = document.object().value(QStringLiteral("vehicle")).toString().trimmed();
        if (!name.isEmpty())
            setVehicleName(name);
        return;
    }

    if (trimmed.size() < 96) {
        const QString text = QString::fromUtf8(trimmed);
        if (!text.contains(QLatin1Char('\n')))
            setVehicleName(text);
    }
}

void BleTelemetrySource::onServiceError(QLowEnergyService::ServiceError error)
{
    if (m_stopped)
        return;

    Q_UNUSED(error);
    fail(QStringLiteral("BLE service error"));
}

void BleTelemetrySource::reconnect()
{
    if (m_stopped)
        return;

    cleanupConnection();
    startScan();
}

void BleTelemetrySource::cleanupConnection()
{
    m_scanning = false;
    m_connecting = false;

    if (m_discovery && m_discovery->isActive())
        m_discovery->stop();

    if (m_service) {
        QObject::disconnect(m_service, nullptr, this, nullptr);
        m_service->deleteLater();
        m_service = nullptr;
    }

    if (m_controller) {
        QObject::disconnect(m_controller, nullptr, this, nullptr);
        m_controller->disconnectFromDevice();
        m_controller->deleteLater();
        m_controller = nullptr;
    }
}

void BleTelemetrySource::fail(const QString &message)
{
    if (m_stopped)
        return;

    setState(State::Error, message);
    cleanupConnection();
    m_reconnectTimer.start();
}

bool BleTelemetrySource::matches(const QBluetoothDeviceInfo &info, const QString &filter)
{
    const bool uuidMatch = info.serviceUuids().contains(telemetryService());

    if (!filter.isEmpty())
        return info.name().contains(filter, Qt::CaseInsensitive);

    return uuidMatch || info.name().contains(QStringLiteral("DashView"), Qt::CaseInsensitive);
}
