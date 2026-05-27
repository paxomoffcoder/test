#include "databasemanager.h"
#include "../utils/logger.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QDir>
#include <QThread>

static void configureSqliteConnection(QSqlDatabase& db)
{
    if (!db.isValid() || !db.isOpen())
    {
        return;
    }

    QSqlQuery pragmaQuery(db);
    pragmaQuery.exec("PRAGMA journal_mode = WAL");
    pragmaQuery.exec("PRAGMA synchronous = NORMAL");
    pragmaQuery.exec("PRAGMA busy_timeout = 30000");  // 30 секунд для lockwait
    pragmaQuery.exec("PRAGMA temp_store = MEMORY");
    pragmaQuery.exec("PRAGMA cache_size = -64000");  // 64MB кэша
}

static DatabaseManager* dbInstance = nullptr;

DatabaseManager* DatabaseManager::instance()
{
    if (!dbInstance)
    {
        dbInstance = new DatabaseManager();
    }
    return dbInstance;
}

DatabaseManager::DatabaseManager()
{
}

bool DatabaseManager::initialize(const QString& dbPath)
{
    DatabaseManager* mgr = instance();
    {
        QMutexLocker locker(&mgr->dbMutex);
        Logger::info(QString("Initializing database at: %1").arg(dbPath));

        // Создаём директорию, если её нет
        QDir dir(QFileInfo(dbPath).dir());
        if (!dir.exists())
        {
            if (!dir.mkpath("."))
            {
                Logger::error("Failed to create database directory");
                return false;
            }
        }

        mgr->dbPath = dbPath;
    }

    // Получаем соединение для текущего потока (main thread)
    QSqlDatabase db = getDatabase();
    if (!db.isValid() || !db.isOpen())
    {
        Logger::error("Failed to get database connection during initialization");
        return false;
    }

    Logger::info("Database connection established");

    // Создание схемы
    if (!createSchema())
    {
        Logger::error("Failed to create database schema");
        close();
        return false;
    }
    
    Logger::info("Database schema created successfully");
    return true;
}

QSqlDatabase DatabaseManager::getDatabase()
{
    // Уникальное имя соединения для текущего потока
    QString connName = QString("db_%1").arg(reinterpret_cast<quintptr>(QThread::currentThreadId()));
    
    // Если соединение для этого потока ещё не создано — создаём
    if (!QSqlDatabase::contains(connName))
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(instance()->dbPath);
        
        if (!db.open())
        {
            Logger::error(QString("Failed to open DB in thread %1: %2")
                .arg(connName).arg(db.lastError().text()));
            return QSqlDatabase();
        }
        
        // Применяем оптимизации к новому соединению
        configureSqliteConnection(db);
        Logger::info(QString("Database connection created for thread %1").arg(connName));
    }
    
    // Возвращаем соединение для текущего потока
    QSqlDatabase db = QSqlDatabase::database(connName, false);
    if (!db.isValid() || !db.isOpen())
    {
        Logger::error(QString("Connection %1 is not valid").arg(connName));
        return QSqlDatabase();
    }
    
    return db;
}

bool DatabaseManager::isConnected()
{
    QSqlDatabase db = getDatabase();
    return db.isValid() && db.isOpen();
}

void DatabaseManager::close()
{
    DatabaseManager* mgr = instance();
    QMutexLocker locker(&mgr->dbMutex);

    // Закрываем глобальное соединение (если осталось)
    if (QSqlDatabase::contains("messenger_db_global"))
    {
        QSqlDatabase::database("messenger_db_global").close();
        QSqlDatabase::removeDatabase("messenger_db_global");
    }
    
    // Закрываем все соединения, созданные для потоков
    for (const QString& connName : QSqlDatabase::connectionNames())
    {
        if (connName.startsWith("db_"))
        {
            QSqlDatabase::database(connName).close();
            QSqlDatabase::removeDatabase(connName);
        }
    }
    
    Logger::info("All database connections closed");
}

bool DatabaseManager::createSchema()
{
    QSqlDatabase db = getDatabase();
    if (!db.isValid() || !db.isOpen())
    {
        Logger::error("Database not available for schema creation");
        return false;
    }
    
    // Таблица users
    QSqlQuery query(db);
    if (!query.exec(
        "CREATE TABLE IF NOT EXISTS users ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  login TEXT UNIQUE NOT NULL,"
        "  password_hash TEXT NOT NULL,"
        "  nickname TEXT NOT NULL,"
        "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  last_seen DATETIME DEFAULT CURRENT_TIMESTAMP"
        ")"
    ))
    {
        Logger::error(QString("Failed to create users table: %1").arg(query.lastError().text()));
        return false;
    }
    
    // Индекс по логину для быстрой авторизации
    if (!query.exec("CREATE UNIQUE INDEX IF NOT EXISTS idx_users_login ON users(login)"))
    {
        Logger::error(QString("Failed to create idx_users_login: %1").arg(query.lastError().text()));
        return false;
    }
    
    // Таблица chats
    if (!query.exec(
        "CREATE TABLE IF NOT EXISTS chats ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  chat_type TEXT NOT NULL CHECK (chat_type IN ('private', 'group')),"
        "  name TEXT,"
        "  creator_id INTEGER NOT NULL,"
        "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  llm_enabled BOOLEAN DEFAULT 0,"
        "  FOREIGN KEY(creator_id) REFERENCES users(id)"
        ")"
    ))
    {
        Logger::error(QString("Failed to create chats table: %1").arg(query.lastError().text()));
        return false;
    }
    
    // Таблица chat_members (многие-ко-многим)
    if (!query.exec(
        "CREATE TABLE IF NOT EXISTS chat_members ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  chat_id INTEGER NOT NULL,"
        "  user_id INTEGER NOT NULL,"
        "  role TEXT DEFAULT 'member' CHECK (role IN ('member', 'admin', 'owner')),"
        "  joined_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  notifications_disabled BOOLEAN DEFAULT 0,"
        "  UNIQUE(chat_id, user_id),"
        "  FOREIGN KEY(chat_id) REFERENCES chats(id),"
        "  FOREIGN KEY(user_id) REFERENCES users(id)"
        ")"
    ))
    {
        Logger::error(QString("Failed to create chat_members table: %1").arg(query.lastError().text()));
        return false;
    }
    
    // Индекс для быстрого поиска участников чата
    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_chat_members_chat ON chat_members(chat_id)"))
    {
        Logger::error(QString("Failed to create idx_chat_members_chat: %1").arg(query.lastError().text()));
        return false;
    }
    
    // Таблица messages
    if (!query.exec(
        "CREATE TABLE IF NOT EXISTS messages ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  chat_id INTEGER NOT NULL,"
        "  sender_id INTEGER NOT NULL,"
        "  author_type TEXT DEFAULT 'user' CHECK (author_type IN ('user', 'llm', 'system')),"
        "  text_content TEXT NOT NULL,"
        "  sent_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  edited_at DATETIME,"
        "  client_message_id TEXT,"
        "  FOREIGN KEY(chat_id) REFERENCES chats(id),"
        "  FOREIGN KEY(sender_id) REFERENCES users(id)"
        ")"
    ))
    {
        Logger::error(QString("Failed to create messages table: %1").arg(query.lastError().text()));
        return false;
    }
    
    // Индекс для быстрой пагинации истории по чатам
    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_messages_chat_time ON messages(chat_id, sent_at)"))
    {
        Logger::error(QString("Failed to create idx_messages_chat_time: %1").arg(query.lastError().text()));
        return false;
    }
    
    // Индекс для идемпотентности (client_message_id)
    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_messages_client_id ON messages(client_message_id)"))
    {
        Logger::error(QString("Failed to create idx_messages_client_id: %1").arg(query.lastError().text()));
        return false;
    }
    
    // Таблица read_state (отслеживание прочтения)
    if (!query.exec(
        "CREATE TABLE IF NOT EXISTS read_state ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  chat_id INTEGER NOT NULL,"
        "  user_id INTEGER NOT NULL,"
        "  last_read_message_id INTEGER,"
        "  unread_count INTEGER DEFAULT 0,"
        "  updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  UNIQUE(chat_id, user_id),"
        "  FOREIGN KEY(chat_id) REFERENCES chats(id),"
        "  FOREIGN KEY(user_id) REFERENCES users(id)"
        ")"
    ))
    {
        Logger::error(QString("Failed to create read_state table: %1").arg(query.lastError().text()));
        return false;
    }
    
    // Индекс для быстрого обновления счётчика непрочитанных
    if (!query.exec("CREATE UNIQUE INDEX IF NOT EXISTS idx_read_state_chat_user ON read_state(chat_id, user_id)"))
    {
        Logger::error(QString("Failed to create idx_read_state_chat_user: %1").arg(query.lastError().text()));
        return false;
    }
    
    // Таблица files (метаданные вложений)
    if (!query.exec(
        "CREATE TABLE IF NOT EXISTS files ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  message_id INTEGER NOT NULL,"
        "  original_filename TEXT NOT NULL,"
        "  stored_path TEXT NOT NULL,"
        "  mime_type TEXT,"
        "  file_size INTEGER,"
        "  checksum TEXT,"
        "  uploaded_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  FOREIGN KEY(message_id) REFERENCES messages(id)"
        ")"
    ))
    {
        Logger::error(QString("Failed to create files table: %1").arg(query.lastError().text()));
        return false;
    }
    
    // Индекс для быстрого поиска файлов по сообщениям
    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_files_message ON files(message_id)"))
    {
        Logger::error(QString("Failed to create idx_files_message: %1").arg(query.lastError().text()));
        return false;
    }
    
    // Таблица event_logs (логирование событий)
    if (!query.exec(
        "CREATE TABLE IF NOT EXISTS event_logs ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  event_type TEXT NOT NULL,"
        "  severity TEXT DEFAULT 'INFO' CHECK (severity IN ('DEBUG', 'INFO', 'WARN', 'ERROR')),"
        "  user_id INTEGER,"
        "  message TEXT,"
        "  metadata TEXT,"
        "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  FOREIGN KEY(user_id) REFERENCES users(id)"
        ")"
    ))
    {
        Logger::error(QString("Failed to create event_logs table: %1").arg(query.lastError().text()));
        return false;
    }
    
    // Индекс для быстрого выбора последних событий
    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_logs_time_severity ON event_logs(created_at DESC, severity)"))
    {
        Logger::error(QString("Failed to create idx_logs_time_severity: %1").arg(query.lastError().text()));
        return false;
    }
    
    Logger::info("All database tables and indexes created successfully");
    return true;
}
