#include "servercore.h"
#include "coreconfig.h"
#include "../network/httpserver.h"
#include "../database/databasemanager.h"
#include "../utils/logger.h"
#include <QCoreApplication>
#include <QDateTime>
#include <signal.h>
#include <iostream>

// Глобальный указатель для обработчика сигналов
static ServerCore* g_serverCore = nullptr;

void signalHandler(int signum)
{
    if (g_serverCore)
    {
        std::cout << "\nReceived signal " << signum << ", shutting down gracefully..." << std::endl;
        g_serverCore->shutdown();
    }
}

ServerCore::ServerCore(QObject* parent)
    : QObject(parent), httpServer(nullptr), running(false)
{
    g_serverCore = this;
}

ServerCore::~ServerCore()
{
    if (httpServer)
    {
        httpServer->stop();
        delete httpServer;
    }
}

bool ServerCore::initialize()
{
    Logger::info("========== Messenger Server Initialization ==========");
    Logger::info(QString("Time: %1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
    Logger::info(QString("Version: 1.0.0"));
    
    // Шаг 1: Конфигурация
    if (!initializeConfig())
    {
        Logger::error("Failed to initialize configuration");
        return false;
    }
    
    // Шаг 2: Логирование
    if (!initializeLogging())
    {
        Logger::error("Failed to initialize logging");
        return false;
    }
    
    // Шаг 3: База данных
    if (!initializeDatabase())
    {
        Logger::error("Failed to initialize database");
        return false;
    }
    
    // Шаг 4: HTTP сервер
    if (!initializeHttpServer())
    {
        Logger::error("Failed to initialize HTTP server");
        return false;
    }
    
    // Регистрируем обработчики сигналов
    registerSignalHandlers();
    
    Logger::info("========== Server Initialization Complete ==========");
    return true;
}

int ServerCore::run()
{
    if (!initialize())
    {
        Logger::error("Server initialization failed");
        return 1;
    }
    
    running = true;
    
    Logger::info("Starting server event loop...");
    Logger::info(QString("Server is ready to accept connections on port %1").arg(CoreConfig::getServerPort()));
    
    QCoreApplication* app = QCoreApplication::instance();
    if (!app)
    {
        Logger::error("QCoreApplication instance not found");
        return 1;
    }
    
    // Запускаем главный цикл событий Qt
    int exitCode = app->exec();
    
    Logger::info(QString("Server stopped with exit code: %1").arg(exitCode));
    return exitCode;
}

void ServerCore::shutdown()
{
    if (!running)
    {
        return;
    }
    
    running = false;
    Logger::info("Beginning graceful shutdown...");
    
    // Останавливаем HTTP сервер
    if (httpServer)
    {
        httpServer->stop();
    }
    
    // Закрываем БД
    DatabaseManager::close();
    
    // Закрываем логгер
    Logger::close();
    
    Logger::info("Graceful shutdown complete");
    
    // Выходим из главного цикла
    QCoreApplication::quit();
}

bool ServerCore::initializeConfig()
{
    Logger::info("Initializing configuration...");
    
    CoreConfig::init();
    
    if (!CoreConfig::validate())
    {
        Logger::error("Configuration validation failed");
        return false;
    }
    
    Logger::info(QString("Server port: %1").arg(CoreConfig::getServerPort()));
    Logger::info(QString("Database: %1").arg(CoreConfig::getDatabasePath()));
    Logger::info(QString("Logging level: %1").arg(CoreConfig::getLoggingLevel()));
    
    return true;
}

bool ServerCore::initializeLogging()
{
    Logger::info("Initializing logging...");
    
    QString logFile = CoreConfig::getLogFilePath();
    QString levelStr = CoreConfig::getLoggingLevel();
    
    Logger::LogLevel level = Logger::INFO;
    if (levelStr == "DEBUG")
        level = Logger::DEBUG;
    else if (levelStr == "WARN")
        level = Logger::WARN;
    else if (levelStr == "ERROR")
        level = Logger::ERROR;
    
    Logger::init(logFile, level);
    Logger::info("Logging initialized successfully");
    
    return true;
}

bool ServerCore::initializeDatabase()
{
    Logger::info("Initializing database...");
    
    QString dbPath = CoreConfig::getDatabasePath();
    
    if (!DatabaseManager::initialize(dbPath))
    {
        Logger::error("Failed to initialize database manager");
        return false;
    }
    
    if (!DatabaseManager::isConnected())
    {
        Logger::error("Database connection check failed");
        return false;
    }
    
    Logger::info("Database initialized successfully");
    return true;
}

bool ServerCore::initializeHttpServer()
{
    Logger::info("Initializing HTTP server...");
    
    httpServer = new HttpServer(this);
    
    quint16 port = CoreConfig::getServerPort();
    
    if (!httpServer->start(port))
    {
        Logger::error(QString("Failed to start HTTP server on port %1").arg(port));
        return false;
    }
    
    Logger::info(QString("HTTP server listening on port %1").arg(port));
    return true;
}

void ServerCore::registerSignalHandlers()
{
    #ifdef Q_OS_UNIX
    // Unix-подобные системы (Linux, macOS)
    signal(SIGINT, signalHandler);   // Ctrl+C
    signal(SIGTERM, signalHandler);  // Сигнал завершения
    #endif
    
    #ifdef Q_OS_WIN
    // Windows
    signal(SIGINT, signalHandler);   // Ctrl+C
    signal(SIGTERM, signalHandler);  // Сигнал завершения
    #endif
    
    Logger::info("Signal handlers registered");
}

void ServerCore::onShutdownSignal()
{
    Logger::info("Shutdown signal received");
    shutdown();
}
