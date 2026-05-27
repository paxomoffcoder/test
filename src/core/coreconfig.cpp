#include "coreconfig.h"
#include "../utils/logger.h"
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QDir>

static CoreConfig* configInstance = nullptr;

CoreConfig* CoreConfig::instance()
{
    if (!configInstance)
    {
        configInstance = new CoreConfig();
    }
    return configInstance;
}

CoreConfig::CoreConfig()
{
}

void CoreConfig::init()
{
    CoreConfig* cfg = instance();
    
    Logger::info("Initializing CoreConfig with default values");
    
    // Встроенные значения по умолчанию (Слой 1)
    cfg->config["server.port"] = "8080";
    cfg->config["server.host"] = "0.0.0.0";
    cfg->config["db.path"] = "./data/messenger.db";
    cfg->config["logging.level"] = "INFO";
    cfg->config["logging.file"] = "./logs/server.log";
    cfg->config["request.timeout_ms"] = "30000";
    cfg->config["limits.max_connections"] = "1000";
    cfg->config["limits.polling_interval_sec"] = "3";
    
    // Загружаем переменные окружения (Слой 2)
    loadFromEnvironment();
}

QString CoreConfig::get(const QString& key, const QString& defaultValue)
{
    CoreConfig* cfg = instance();
    auto it = cfg->config.find(key);
    
    if (it != cfg->config.end())
    {
        return it.value();
    }
    
    return defaultValue;
}

int CoreConfig::getInt(const QString& key, int defaultValue)
{
    QString value = get(key, "");
    
    if (value.isEmpty())
    {
        return defaultValue;
    }
    
    bool ok;
    int result = value.toInt(&ok);
    
    if (ok)
    {
        return result;
    }
    
    Logger::warn(QString("Failed to convert config value to int: %1 = %2").arg(key).arg(value));
    return defaultValue;
}

void CoreConfig::set(const QString& key, const QString& value)
{
    CoreConfig* cfg = instance();
    cfg->config[key] = value;
    Logger::debug(QString("Config set: %1 = %2").arg(key).arg(value));
}

void CoreConfig::loadFromEnvironment()
{
    CoreConfig* cfg = instance();
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    
    Logger::info("Loading configuration from environment variables");
    
    // Маппируем переменные окружения на параметры конфигурации
    QMap<QString, QString> envMap = {
        {"MESSENGER_SERVER_PORT", "server.port"},
        {"MESSENGER_SERVER_HOST", "server.host"},
        {"MESSENGER_DB_PATH", "db.path"},
        {"MESSENGER_LOG_LEVEL", "logging.level"},
        {"MESSENGER_LOG_FILE", "logging.file"},
        {"MESSENGER_REQUEST_TIMEOUT", "request.timeout_ms"},
        {"MESSENGER_MAX_CONNECTIONS", "limits.max_connections"},
        {"MESSENGER_POLLING_INTERVAL", "limits.polling_interval_sec"},
    };
    
    for (auto it = envMap.begin(); it != envMap.end(); ++it)
    {
        QString envVar = it.key();
        QString configKey = it.value();
        
        if (env.contains(envVar))
        {
            QString value = env.value(envVar);
            cfg->config[configKey] = value;
            Logger::info(QString("Config from env: %1 = %2").arg(configKey).arg(value));
        }
    }
}

bool CoreConfig::validate()
{
    Logger::info("Validating configuration");
    
    // Проверяем порт
    int port = getServerPort();
    if (port < 1024 || port > 65535)
    {
        Logger::error(QString("Invalid server port: %1 (must be 1024-65535)").arg(port));
        return false;
    }
    
    // Проверяем путь к БД
    QString dbPath = getDatabasePath();
    if (dbPath.isEmpty())
    {
        Logger::error("Database path is empty");
        return false;
    }
    
    // Проверяем уровень логирования
    QString logLevel = getLoggingLevel();
    if (logLevel != "DEBUG" && logLevel != "INFO" && logLevel != "WARN" && logLevel != "ERROR")
    {
        Logger::error(QString("Invalid logging level: %1").arg(logLevel));
        return false;
    }
    
    // Проверяем таймаут
    int timeout = getRequestTimeout();
    if (timeout < 1000 || timeout > 300000)
    {
        Logger::error(QString("Invalid request timeout: %1 (must be 1000-300000 ms)").arg(timeout));
        return false;
    }
    
    Logger::info("Configuration validation passed");
    return true;
}

quint16 CoreConfig::getServerPort()
{
    return getInt("server.port", 8080);
}

QString CoreConfig::getDatabasePath()
{
    return get("db.path", "./data/messenger.db");
}

QString CoreConfig::getLogFilePath()
{
    return get("logging.file", "./logs/server.log");
}

QString CoreConfig::getLoggingLevel()
{
    return get("logging.level", "INFO");
}

int CoreConfig::getRequestTimeout()
{
    return getInt("request.timeout_ms", 30000);
}
