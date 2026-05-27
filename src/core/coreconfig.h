#ifndef CORECONFIG_H
#define CORECONFIG_H

#include <QString>
#include <QMap>

/**
 * @class CoreConfig
 * @brief Управление конфигурацией сервера
 *
 * Загружает параметры из иерархии источников:
 * 1. Встроенные значения (код)
 * 2. Переменные окружения
 * 3. База данных (наивысший приоритет)
 *
 * Параметры:
 * - server.port - порт прослушивания (по умолчанию 8080)
 * - db.path - путь к файлу SQLite (по умолчанию ./data/messenger.db)
 * - logging.level - уровень логирования (DEBUG, INFO, WARN, ERROR)
 * - logging.file - путь к файлу логов (по умолчанию ./logs/server.log)
 * - request.timeout_ms - таймаут обработки запроса в миллисекундах
 */
class CoreConfig
{
public:
    /**
     * Инициализация конфигурации
     * Загружает встроенные значения по умолчанию
     */
    static void init();

    /**
     * Получение параметра конфигурации
     * @param key - название параметра (например, "server.port")
     * @param defaultValue - значение по умолчанию
     * @return значение параметра
     */
    static QString get(const QString& key, const QString& defaultValue = "");

    /**
     * Получение целого значения параметра
     */
    static int getInt(const QString& key, int defaultValue = 0);

    /**
     * Установка параметра
     */
    static void set(const QString& key, const QString& value);

    /**
     * Загрузка конфигурации из переменных окружения
     */
    static void loadFromEnvironment();

    /**
     * Валидация конфигурации
     * @return true если все параметры корректны
     */
    static bool validate();

    /**
     * Получить порт сервера
     */
    static quint16 getServerPort();

    /**
     * Получить путь к БД
     */
    static QString getDatabasePath();

    /**
     * Получить путь к файлу логов
     */
    static QString getLogFilePath();

    /**
     * Получить уровень логирования
     */
    static QString getLoggingLevel();

    /**
     * Получить таймаут запроса в миллисекундах
     */
    static int getRequestTimeout();

private:
    CoreConfig();

    static CoreConfig* instance();

    QMap<QString, QString> config;
};

#endif // CORECONFIG_H
