#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QString>
#include <QThreadPool>

/**
 * @class HttpServer
 * @brief HTTP сервер для прослушивания входящих соединений
 *
 * Отвечает за:
 * - Прослушивание входящих TCP соединений
 * - Чтение HTTP запросов
 * - Маршрутизация запросов на нужные обработчики
 * - Параллельная обработка запросов в пуле потоков
 */
class HttpServer : public QTcpServer
{
    Q_OBJECT

    friend class RequestProcessor;

public:
    explicit HttpServer(QObject* parent = nullptr);
    
    /**
     * Запуск сервера на указанном порте
     * @param port - порт прослушивания
     * @return true если сервер успешно запущен
     */
    bool start(quint16 port);

    /**
     * Остановка сервера
     */
    void stop();

    /**
     * Получить порт сервера
     */
    quint16 getPort() const;

protected:
    /**
     * Слот Qt для обработки новых входящих соединений
     */
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    /**
     * Слот для обработки готовности данных к чтению
     */
    void onReadyRead();

    /**
     * Слот для обработки отключения клиента
     */
    void onDisconnected();

    /**
     * Слот для обработки ошибок сокета
     */
    void onSocketError(QAbstractSocket::SocketError socketError);

private:
    /**
     * Обработка HTTP запроса (маршрутизация)
     */
    void processRequest(QTcpSocket* socket, const QString& request);

    /**
     * Маршрутизация POST запросов
     */
    void routePostRequest(class HttpRequestHandler* handler, const QString& path);

    /**
     * Маршрутизация GET запросов
     */
    void routeGetRequest(class HttpRequestHandler* handler, const QString& path);

    /**
     * Маршрутизация PUT запросов
     */
    void routePutRequest(class HttpRequestHandler* handler, const QString& path);

    /**
     * Маршрутизация DELETE запросов
     */
    void routeDeleteRequest(class HttpRequestHandler* handler, const QString& path);

    /**
     * Извлечение ID из пути (например, из "/api/messages/5" вернуть 5)
     */
    static int extractIdFromPath(const QString& path);

    QThreadPool threadPool;
};

#endif // HTTPSERVER_H
