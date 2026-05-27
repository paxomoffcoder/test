#ifndef SERVERCORE_H
#define SERVERCORE_H

#include <QString>
#include <QObject>

class HttpServer;

/**
 * @class ServerCore
 * @brief Главный класс сервера
 *
 * Отвечает за:
 * - Инициализацию всех компонентов (конфиг, БД, HTTP сервер)
 * - Запуск главного цикла
 * - Graceful shutdown (корректное завершение)
 * - Обработка сигналов (SIGINT, SIGTERM)
 */
class ServerCore : public QObject
{
    Q_OBJECT

public:
    explicit ServerCore(QObject* parent = nullptr);
    ~ServerCore();

    /**
     * Инициализация сервера
     * @return true если инициализация успешна
     */
    bool initialize();

    /**
     * Запуск сервера и главного цикла
     * @return код завершения
     */
    int run();

    /**
     * Остановка сервера
     */
    void shutdown();

private:
    HttpServer* httpServer;
    bool running;

    /**
     * Инициализация конфигурации
     */
    bool initializeConfig();

    /**
     * Инициализация логирования
     */
    bool initializeLogging();

    /**
     * Инициализация базы данных
     */
    bool initializeDatabase();

    /**
     * Инициализация HTTP сервера
     */
    bool initializeHttpServer();

    /**
     * Регистрация обработчиков сигналов
     */
    void registerSignalHandlers();

private slots:
    /**
     * Слот для обработки сигнала завершения
     */
    void onShutdownSignal();
};

#endif // SERVERCORE_H
