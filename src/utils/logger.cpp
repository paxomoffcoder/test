#include "logger.h"
#include <QDateTime>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <iostream>

static Logger* loggerInstance = nullptr;

Logger* Logger::instance()
{
    if (!loggerInstance)
    {
        loggerInstance = new Logger();
    }
    return loggerInstance;
}

Logger::Logger() : currentLevel(INFO)
{
}

void Logger::init(const QString& logFile, LogLevel level)
{
    Logger* log = instance();
    log->currentLevel = level;

    QFileInfo fileInfo(logFile);
    QString absoluteLogFilePath = fileInfo.isAbsolute()
        ? fileInfo.absoluteFilePath()
        : QDir::current().absoluteFilePath(logFile);

    QDir logDir;
    if (!logDir.mkpath(QFileInfo(absoluteLogFilePath).absolutePath()))
    {
        std::cerr << "Failed to create log directory for: " << absoluteLogFilePath.toStdString() << std::endl;
        return;
    }

    log->logFile.setFileName(absoluteLogFilePath);
    
    if (!log->logFile.open(QIODevice::Append | QIODevice::Text))
    {
        std::cerr << "Failed to open log file: " << absoluteLogFilePath.toStdString() << std::endl;
        return;
    }
    
    log->logStream.setDevice(&log->logFile);
    log->info("Logger initialized");
}

void Logger::debug(const QString& message)
{
    instance()->logMessage(DEBUG, message);
}

void Logger::info(const QString& message)
{
    instance()->logMessage(INFO, message);
}

void Logger::warn(const QString& message)
{
    instance()->logMessage(WARN, message);
}

void Logger::error(const QString& message)
{
    instance()->logMessage(ERROR, message);
}

void Logger::setLevel(LogLevel level)
{
    instance()->currentLevel = level;
}

void Logger::logMessage(LogLevel level, const QString& message)
{
    Logger* log = instance();
    
    // Проверяем, нужно ли логировать это сообщение
    if (level > log->currentLevel)
    {
        return;
    }
    
    QString levelStr;
    switch (level)
    {
        case DEBUG: levelStr = "DEBUG"; break;
        case INFO:  levelStr = "INFO";  break;
        case WARN:  levelStr = "WARN";  break;
        case ERROR: levelStr = "ERROR"; break;
    }
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString formattedMessage = QString("[%1] %2: %3").arg(timestamp, levelStr, message);
    
    // Логирование в консоль
    std::cout << formattedMessage.toStdString() << std::endl;
    
    // Логирование в файл (если файл открыт)
    {
        QMutexLocker locker(&log->logMutex);
        if (log->logFile.isOpen())
        {
            log->logStream << formattedMessage << "\n";
            log->logStream.flush();
        }
    }
}

void Logger::close()
{
    Logger* log = instance();
    {
        QMutexLocker locker(&log->logMutex);
        if (log->logFile.isOpen())
        {
            log->logStream << "[" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") 
                          << "] INFO: Logger closed\n";
            log->logFile.close();
        }
    }
}
