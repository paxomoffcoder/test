#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMutex>

/**
 * @class Logger
 * @brief Система логирования с поддержкой уровней
 *
 * Поддерживает уровни логирования:
 * - DEBUG (4)
 * - INFO (3) - по умолчанию
 * - WARN (2)
 * - ERROR (1)
 *
 * Логирует как в консоль, так и в файл
 */
class Logger
{
public:
    enum LogLevel
    {
        DEBUG = 4,
        INFO = 3,
        WARN = 2,
        ERROR = 1
    };

    // Инициализация логгера с файлом и уровнем
    static void init(const QString& logFile, LogLevel level = INFO);
    
    // Методы логирования
    static void debug(const QString& message);
    static void info(const QString& message);
    static void warn(const QString& message);
    static void error(const QString& message);
    
    // Установка уровня логирования во время работы
    static void setLevel(LogLevel level);
    
    // Закрытие логгера
    static void close();

private:
    Logger();
    
    static void logMessage(LogLevel level, const QString& message);
    
    static Logger* instance();
    
    QFile logFile;
    QTextStream logStream;
    LogLevel currentLevel;
    QMutex logMutex;
};

#endif // LOGGER_H
