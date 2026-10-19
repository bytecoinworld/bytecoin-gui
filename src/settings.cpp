// Copyright (c) 2015-2018, The Bytecoin developers.
// Licensed under the GNU Lesser General Public License. See LICENSE for details.

#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QApplication>
#include <type_traits>

#include "settings.h"
#include "common.h"
#include "version.h"

namespace WalletGUI
{

namespace
{

constexpr char OPTION_WALLET_FILE[] = "walletFile";
constexpr char OPTION_LOCAL_RPC_PORT[] = "localRpcPort"; // obsolete
constexpr char OPTION_REMOTE_RPC_END_POINT[] = "remoteRpcEndPoint"; // obsolete
constexpr char OPTION_LOCAL_WALLETD_PORT[] = "localWalletdPort";
constexpr char OPTION_REMOTE_WALLETD_END_POINT[] = "remoteWalletdEndPoint";
constexpr char OPTION_WALLETD_PROFILES[] = "walletdProfiles";
constexpr char OPTION_CONNECTION_METHOD[] = "connectionMethod"; // obsolete
constexpr char OPTION_WALLETD_CONNECTION_METHOD[] = "walletdConnectionMethod";
constexpr char OPTION_BYTECOIND_CONNECTION_METHOD[] = "bytecoindConnectionMethod";
constexpr char OPTION_BYTECOIND_HOST[] = "bytecoindHost";
constexpr char OPTION_BYTECOIND_PORT[] = "bytecoindPort";
constexpr char OPTION_PUBLIC_BYTECOIND_NODES[] = "publicBytecoindNodes";
constexpr char OPTION_NETWORK_TYPE[] = "networkType";
constexpr char OPTION_DAY_MODE[] = "dayMode";
constexpr char OPTION_RECENT_WALLETS[] = "recentWallets";
constexpr char OPTION_WALLETD_PARAMS[] = "walletdParams";

constexpr quint16 DEFAULT_MAIN_WALLETD_RPC_PORT = 8070;
constexpr quint16 DEFAULT_TEST_WALLETD_RPC_PORT = DEFAULT_MAIN_WALLETD_RPC_PORT + 1000;
constexpr quint16 DEFAULT_STAGE_WALLETD_RPC_PORT = DEFAULT_MAIN_WALLETD_RPC_PORT + 2000;

constexpr quint16 DEFAULT_MAIN_BYTECOIND_RPC_PORT = 8081;
constexpr quint16 DEFAULT_TEST_BYTECOIND_RPC_PORT = DEFAULT_MAIN_BYTECOIND_RPC_PORT + 1000;
constexpr quint16 DEFAULT_STAGE_BYTECOIND_RPC_PORT = DEFAULT_MAIN_BYTECOIND_RPC_PORT + 2000;

constexpr char LOCAL_HOST[] = "127.0.0.1";

#if defined(Q_OS_LINUX)
constexpr char DEFAULT_WORK_DIR[] = ".bytecoin";
#endif

static
bool readJsonFile(QIODevice& device, QSettings::SettingsMap& map)
{
    const QJsonDocument json = QJsonDocument::fromJson(device.readAll());
    map = json.object().toVariantMap();
    return true;
}

static
bool writeJsonFile(QIODevice& device, const QSettings::SettingsMap& map)
{
    device.write(QJsonDocument(QJsonObject::fromVariantMap(map)).toJson());
    return true;
}

}

Settings::Settings()
{
    const QSettings::Format jsonFormat = QSettings::registerFormat("json", readJsonFile, writeJsonFile);
    QSettings::setDefaultFormat(jsonFormat);
    makeDataDir(getDefaultWorkDir());
    const QString jsonFile = getDefaultWorkDir().absoluteFilePath("bytecoin-gui.config");

    settings_.reset(new QSettings(jsonFile, jsonFormat));

    renameLegacyParameter(OPTION_CONNECTION_METHOD   , OPTION_WALLETD_CONNECTION_METHOD);
    renameLegacyParameter(OPTION_LOCAL_RPC_PORT      , OPTION_LOCAL_WALLETD_PORT);
    renameLegacyParameter(OPTION_REMOTE_RPC_END_POINT, OPTION_REMOTE_WALLETD_END_POINT);
}

void Settings::renameLegacyParameter(const QString& legacyName, const QString& newName)
{
    if (settings_->contains(legacyName))
    {
        if (!settings_->contains(newName))
        {
            const QVariant& value = settings_->value(legacyName);
            settings_->setValue(newName, value);
        }
        settings_->remove(legacyName);
    }
}

Settings& Settings::instance()
{
    static Settings settings;
    return settings;
}

template<typename EnumT>
EnumT Settings::getEnumValue(const QString& key, EnumT defaultValue) const
{
    static_assert(std::is_enum<EnumT>::value, "EnumT must be an enumeration");
    using underlying_t = std::underlying_type_t<EnumT>;
    return static_cast<EnumT>(qvariant_cast<underlying_t>(settings_->value(key, static_cast<underlying_t>(defaultValue))));
}

QSettings::Format Settings::getFormat() const
{
    return settings_->format();
}

//quint16 Settings::getLocalRpcPort() const
//{
//    return settings_->value(OPTION_LOCAL_RPC_PORT, getDefaultWalletdPort()).toUInt();
//}

quint16 Settings::getLocalWalletdPort() const
{
    return settings_->value(OPTION_LOCAL_WALLETD_PORT, getDefaultWalletdPort()).toUInt();
}

//QString Settings::getRemoteRpcEndPoint() const
//{
//    return settings_->value(OPTION_REMOTE_RPC_END_POINT).toString();
//}

QString Settings::getRemoteWalletdEndPoint() const
{
    return settings_->value(OPTION_REMOTE_WALLETD_END_POINT).toString();
}

QVariantMap Settings::getWalletdProfiles() const
{
    return settings_->value(OPTION_WALLETD_PROFILES).toMap();
}

QString Settings::getLocalWalletdEndPoint() const
{
    return QString("%1:%2").arg(LOCAL_HOST).arg(getLocalWalletdPort());
}

QString Settings::getBuiltinWalletdEndPoint() const
{
    return QString("%1:%2").arg(LOCAL_HOST).arg(getDefaultWalletdPort());
}

QString Settings::getWalletdEndPoint() const
{
    switch(getWalletdConnectionMethod())
    {
    case ConnectionMethod::BUILTIN: return getBuiltinWalletdEndPoint();
    case ConnectionMethod::LOCAL:   return getLocalWalletdEndPoint();
    case ConnectionMethod::REMOTE:  return getRemoteWalletdEndPoint();
    }
    return QString();
}

ConnectionMethod Settings::getWalletdConnectionMethod() const
{
//    return static_cast<ConnectionMethod>(settings_->value(OPTION_WALLETD_CONNECTION_METHOD, static_cast<int>(getWalletdDefaultConnectionMethod())).toInt());
    return getEnumValue<ConnectionMethod>(OPTION_WALLETD_CONNECTION_METHOD, getWalletdDefaultConnectionMethod());
}

QString Settings::getUserFriendlyWalletdConnectionMethod() const
{
    switch(getWalletdConnectionMethod())
    {
    case ConnectionMethod::BUILTIN:return tr("built-in walletd");
    case ConnectionMethod::LOCAL:   return tr("local walletd");
    case ConnectionMethod::REMOTE:  return getRemoteWalletdEndPoint();
    }
    return QString();
}

ConnectionMethod Settings::getBytecoindConnectionMethod() const
{
//    return static_cast<ConnectionMethod>(settings_->value(OPTION_BYTECOIND_CONNECTION_METHOD, static_cast<int>(getBytecoindDefaultConnectionMethod())).toInt());
    return getEnumValue<ConnectionMethod>(OPTION_BYTECOIND_CONNECTION_METHOD, getBytecoindDefaultConnectionMethod());
}

QString Settings::getBytecoindEndPoint() const
{
    const quint16 port = getBytecoindPort();
    return QString{"%1:%2"}.arg(getBytecoindHost()).arg(port ? port : getDefaultBytecoindPort());
}

QString Settings::getBytecoindHost() const
{
    return settings_->value(OPTION_BYTECOIND_HOST).toString();
}

quint16 Settings::getBytecoindPort() const
{
    return static_cast<quint16>(settings_->value(OPTION_BYTECOIND_PORT, getDefaultBytecoindPort()).toUInt());
}

NetworkType Settings::getNetworkType() const
{
//    return static_cast<NetworkType>(settings_->value(OPTION_NETWORK_TYPE, static_cast<int>(getDefaultNetworkType())).toInt());
    return getEnumValue<NetworkType>(OPTION_NETWORK_TYPE, getDefaultNetworkType());
}

bool Settings::isDayMode() const
{
    return settings_->value(OPTION_DAY_MODE, false).toBool();
}

QString Settings::getNetworkTypeString() const
{
    switch(getNetworkType())
    {
    case NetworkType::MAIN:  return QString{"main"};
    case NetworkType::STAGE: return QString{"stage"};
    case NetworkType::TEST:  return QString{"test"};
    }
    return QString{};
}

QStringList Settings::getRecentWallets() const
{
    return getStringList(OPTION_RECENT_WALLETS, QStringList());
}

QString Settings::getWalletFile() const
{
    return settings_->value(OPTION_WALLET_FILE).toString();
}

QString Settings::getWalletdParams() const
{
    return settings_->value(OPTION_WALLETD_PARAMS).toString();
}

QStringList Settings::getPublicBytecoindNodes() const
{
    return getStringList(OPTION_PUBLIC_BYTECOIND_NODES, QStringList{});
}

quint16 Settings::getDefaultWalletdPort() const
{
    switch(getNetworkType())
    {
    case NetworkType::MAIN:  return DEFAULT_MAIN_WALLETD_RPC_PORT;
    case NetworkType::STAGE: return DEFAULT_STAGE_WALLETD_RPC_PORT;
    case NetworkType::TEST:  return DEFAULT_TEST_WALLETD_RPC_PORT;
    }
    return DEFAULT_MAIN_WALLETD_RPC_PORT;
}

quint16 Settings::getDefaultBytecoindPort() const
{
    switch(getNetworkType())
    {
    case NetworkType::MAIN:  return DEFAULT_MAIN_BYTECOIND_RPC_PORT;
    case NetworkType::STAGE: return DEFAULT_STAGE_BYTECOIND_RPC_PORT;
    case NetworkType::TEST:  return DEFAULT_TEST_BYTECOIND_RPC_PORT;
    }
    return DEFAULT_MAIN_BYTECOIND_RPC_PORT;
}


void Settings::setWalletdParams(const QString& params)
{
    settings_->setValue(OPTION_WALLETD_PARAMS, params);
}

void Settings::setPublicBytecoindNodes(const QStringList& nodes)
{
    settings_->setValue(OPTION_PUBLIC_BYTECOIND_NODES, nodes);
}

//void Settings::setLocalRpcPort(quint16 port)
//{
//    settings_->setValue(OPTION_LOCAL_RPC_PORT, port);
//}

void Settings::setLocalWalletdPort(quint16 port)
{
    settings_->setValue(OPTION_LOCAL_WALLETD_PORT, port);
}

//void Settings::setRemoteRpcEndPoint(const QString& host, quint16 port)
//{
//    settings_->setValue(OPTION_REMOTE_RPC_END_POINT, QString("%1:%2").arg(host).arg(port));
//}

void Settings::setRemoteWalletdEndPoint(const QString& host, quint16 port)
{
    settings_->setValue(OPTION_REMOTE_WALLETD_END_POINT, QString("%1:%2").arg(host).arg(port));
}

void Settings::setWalletdProfiles(const QVariantMap& profiles)
{
    settings_->setValue(OPTION_WALLETD_PROFILES, profiles);
}

void Settings::setBytecoindEndPoint(const QString& host, quint16 port)
{
    settings_->setValue(OPTION_BYTECOIND_HOST, host);
    settings_->setValue(OPTION_BYTECOIND_PORT, port);
//    settings_->setValue(OPTION_BYTECOIND_END_POINT, QString("%1:%2").arg(host).arg(port));
}

void Settings::setWalletdConnectionMethod(ConnectionMethod method)
{
    settings_->setValue(OPTION_WALLETD_CONNECTION_METHOD, static_cast<int>(method));
}

void Settings::setBytecoindConnectionMethod(ConnectionMethod method)
{
    settings_->setValue(OPTION_BYTECOIND_CONNECTION_METHOD, static_cast<int>(method));
}

void Settings::setNetworkType(NetworkType type)
{
    settings_->setValue(OPTION_NETWORK_TYPE, static_cast<int>(type));
}

void Settings::setDayMode(bool enabled)
{
    settings_->setValue(OPTION_DAY_MODE, enabled);
}

void Settings::setWalletFile(const QString& walletFile)
{
    settings_->setValue(OPTION_WALLET_FILE, walletFile);
}

void Settings::setRecentWallets(const QStringList& wallets)
{
    settings_->setValue(OPTION_RECENT_WALLETS, wallets);
}

void Settings::addRecentWallet(const QString& wallet)
{
    QStringList wallets = getRecentWallets();
    wallets.removeOne(wallet);
    wallets.push_front(wallet);
    setRecentWallets(wallets);
}

void Settings::clearRecentWallets()
{
    setRecentWallets(QStringList());
}

/*static*/
QDir Settings::getDefaultWorkDir()
{
#if defined(Q_OS_WIN32)
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#elif defined(Q_OS_MAC)
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#elif defined(Q_OS_LINUX)
    return QDir::homePath() + '/' + DEFAULT_WORK_DIR;
#else
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#endif
}

///*static*/
//QString Settings::getDefaultRpcPath()
//{
//    return DEFAULT_RPC_PATH;
//}

/*static*/
QString Settings::getDefaultWalletdPath()
{
    const QString dir = qApp->applicationDirPath();
#ifdef Q_OS_WIN32
    return dir + '/' + "walletd.exe";
#else
    return dir + '/' + "walletd";
#endif
}

/*static*/
ConnectionMethod Settings::getWalletdDefaultConnectionMethod()
{
    return ConnectionMethod::BUILTIN;
}

/*static*/
ConnectionMethod Settings::getBytecoindDefaultConnectionMethod()
{
    return ConnectionMethod::BUILTIN;
}

/*static*/
NetworkType Settings::getDefaultNetworkType()
{
    return NetworkType::MAIN;
}

/*static*/
void Settings::makeDataDir(const QDir& dataDir)
{
    if (!dataDir.exists())
        dataDir.mkpath(dataDir.absolutePath());
}

/*static*/
QString Settings::getVersion()
{
    return shortVersion();
}

/*static*/
QString Settings::getVersionSuffix()
{
    return VERSION_SUFFIX;
}

/*static*/
QString Settings::getRevision()
{
    return REVISION;
}

/*static*/
QString Settings::getFullVersion()
{
    return fullVersion();
}

QStringList Settings::getStringList(const QString& key, const QVariant& defaultValue) const
{
    return settings_->value(key, defaultValue).value<QStringList>();
}

bool Settings::walletdConnectionMethodSet() const
{
    return settings_->contains(OPTION_WALLETD_CONNECTION_METHOD);
}


}
