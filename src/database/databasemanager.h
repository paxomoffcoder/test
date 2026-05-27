#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QString>
#include <QMutex>

/**
 * @class DatabaseManager
 * @brief Менеджер для управления подключением к БД
 *
 * Отвечает за:
 * - Подключение к SQLite
 * - Создание таблиц и индексов
 * - Обеспечение потокобезопасного доступа
 */
class DatabaseManager
{
public:
    // Инициализация БД
    static bool initialize(const QString& dbPath);
    
    // Получение подключения (потокобезопасное)
    static QSqlDatabase getDatabase();
    
    // Проверка подключения
    static bool isConnected();
    
    // Закрытие БД
    static void close();

private:
    DatabaseManager();
    
    // Создание схемы БД (таблицы, индексы)
    static bool createSchema();
    
    static DatabaseManager* instance();
    
    QSqlDatabase db;
    QMutex dbMutex;
    QString dbPath;
};

#endif // DATABASEMANAGER_H
