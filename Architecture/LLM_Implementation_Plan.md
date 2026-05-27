# План реализации LLM-интеграции — MVP (с учётом финального DB-контракта)

## 0) Границы задачи Codex
Codex реализует **только код LLM-модуля**, работающий с уже подготовленной БД через `DatabaseManager` / `QSqlDatabase`.

Codex **не делает**:
- `CREATE TABLE` / `ALTER TABLE`;
- миграции;
- seed-данные;
- создание служебных пользователей;
- добавление новых полей в существующие таблицы.

БД и миграции делает Тимур. Схема ниже — это контракт для SQL-запросов внутри LLM-модуля.

---

## 1) Минимальный конфигурационный слой (делаем первым)

### 1.1 `LlmConfig` (дефолты)
- `endpoint = "http://localhost:11434"`
- `model = "qwen2.5:4b"`
- `timeout_ms = 180000`
- `temperature = 0.7`
- `max_tokens = 1024`
- `context_window = 20`
- `streaming = false`
- `llm_sender_id` (обязательный параметр для записи LLM-сообщений в `messages.sender_id`)

### 1.2 `LlmConfigManager` (минимальный)
- создаёт дефолтный `LlmConfig`;
- отдаёт текущий конфиг через `current()`;
- валидирует базовые диапазоны;
- пока без runtime update;
- пока без чтения `server_config`.

### 1.3 Требование к использованию конфига
`OllamaProvider` и `LlmService` берут параметры только из `LlmConfig`, без хардкода внутри методов.

---

## 2) Актуальный DB-контракт (использовать только эти поля)

### 2.1 `chats`
Использовать:
- `chats.id`
- `chats.chat_type`
- `chats.llm_enabled`

Не использовать устаревшие имена:
- `chats.type`
- `chats.owner_id`

### 2.2 `messages`
Использовать:
- `messages.id`
- `messages.chat_id`
- `messages.sender_id`
- `messages.author_type`
- `messages.text_content`
- `messages.sent_at`
- `messages.edited_at`
- `messages.client_message_id`

Не использовать устаревшие имена:
- `messages.content`
- `messages.created_at`

### 2.3 `llm_requests`
Таблица создаётся Тимуром; LLM-модуль считает, что она уже существует и содержит:
- `id`
- `chat_id`
- `user_id`
- `prompt`
- `response`
- `model`
- `response_ms`
- `status`
- `error_code`
- `created_at`

---

## 3) Поведение LLM-модуля

### 3.1 Общая цепочка MVP
`ILLMProvider → OllamaProvider → ContextBuilder → LlmService → INSERT llm_requests (+ INSERT messages при success)`

### 3.2 Контекст
`ContextBuilder` читает историю сообщений из `messages` по `chat_id` с полями `author_type`, `text_content`, `sent_at` и формирует массив `{role, content}`.

### 3.3 Запись результата в `llm_requests`
После вызова модели всегда пишется техническая запись:
- `chat_id` — чат, где вызывали LLM;
- `user_id` — инициатор запроса;
- `prompt` — отправленный в модель текст/контекст;
- `response` — ответ модели (или пусто при ошибке);
- `model`;
- `response_ms`;
- `status` = `success` или `error`;
- `error_code` только при ошибке;
- `created_at`.

### 3.4 Запись LLM-сообщения в `messages` (только при success)
Вставлять сообщение в тот же `chat_id`:
- `author_type = 'llm'`
- `text_content = <текст ответа модели>`
- `sent_at = CURRENT_TIMESTAMP`
- `sender_id = LlmConfig.llm_sender_id`

Важно:
- не добавлять `messages.llm_owner_id`;
- не использовать `user_id` инициатора как `sender_id` LLM-сообщения;
- инициатор хранится в `llm_requests.user_id`.

---

## 4) Разделение ответственности

### Тимур (backend core)
- сохраняет user-сообщение в `messages`;
- проверяет `chats.llm_enabled`;
- проверяет, что `chat_type='private'`;
- вызывает `LlmService`;
- доставляет новые сообщения клиенту;
- добавляет/мигрирует таблицы и служебного LLM-пользователя.

### Ты (LLM-модуль)
- `LlmConfig` + `LlmConfigManager`;
- `ILLMProvider` + `OllamaProvider`;
- `ContextBuilder`;
- внутренняя очередь `LlmService`;
- SQL-записи в `llm_requests`;
- SQL-запись LLM-ответа в `messages` при success.

---

## 5) Что запрещено в этом этапе
- Любые изменения схемы БД.
- Любые миграции/seed-данные.
- Добавление поля `llm_owner_id`.
- Использование `event_logs` для LLM.
- Dashboard и аналитика.
- Переписывание HTTP-роутинга и сессий.

---

## 6) Пошаговый план реализации (как делаем в работе)

### Шаг 1. Подготовить каркас конфигурации
1. Создать `LlmConfig` с фиксированными дефолтами из раздела 1.1.
2. Создать `LlmConfigManager` с методами:
   - `LlmConfig current() const;`
   - `bool validate(const LlmConfig&) const;`
3. На старте сервера/модуля: валидировать дефолтный конфиг, при невалидности логировать и не запускать LLM-модуль.

**Результат шага:** все LLM-параметры централизованы и готовы к использованию в коде.

### Шаг 2. Ввести контракт провайдера
1. Создать интерфейс `ILLMProvider`.
2. Зафиксировать DTO результата (например `LlmResult`):
   - `ok`
   - `text`
   - `model`
   - `response_ms`
   - `error_code`
3. Привязать интерфейс к `LlmConfig` (методы провайдера получают/используют параметры из него).

**Результат шага:** `LlmService` может зависеть от абстракции, а не от конкретного HTTP-клиента.

### Шаг 3. Реализовать `OllamaProvider`
1. Реализовать вызов OpenAI-compatible endpoint `/v1/chat/completions`.
2. Формировать payload из `LlmConfig` (`model`, `temperature`, `max_tokens`, `streaming`).
3. Использовать `timeout_ms` из `LlmConfig`.
4. Измерять время запроса и писать в `response_ms`.
5. Нормализовать ошибки в `error_code` (`timeout`, `network_error`, `invalid_json`, `provider_5xx`).

**Результат шага:** изолированный провайдер даёт либо корректный ответ, либо стандартизированную ошибку.

### Шаг 4. Реализовать `ContextBuilder`
1. Добавить SQL-чтение последних `N=context_window` сообщений из `messages` по `chat_id`.
2. Использовать только актуальные поля: `author_type`, `text_content`, `sent_at`.
3. Преобразовывать строки в роли LLM (`user` / `assistant` / `system`).
4. Возвращать массив `{role, content}` для провайдера.

**Результат шага:** стабильный контекст для генерации без завязки на старые имена колонок.

### Шаг 5. Реализовать `LlmService` с внутренней очередью
1. Сделать публичный метод `enqueue(chatId, userId, userPrompt)`.
2. Внутри держать FIFO-очередь задач и 1 активную генерацию (MVP).
3. На задаче:
   - собрать контекст через `ContextBuilder`;
   - вызвать `ILLMProvider`;
   - записать результат в БД.
4. Не менять HTTP-роутинг/сессии — этот слой вызывает Тимур.

**Результат шага:** единая точка LLM-обработки, безопасная к bursts-запросам.

### Шаг 6. Реализовать запись в `llm_requests`
1. Добавить метод в `DatabaseManager` (или рядом) для `INSERT` в `llm_requests`.
2. Заполнять поля строго по контракту:
   - `chat_id`, `user_id`, `prompt`, `response`, `model`, `response_ms`, `status`, `error_code`, `created_at`.
3. Статусы только `success`/`error`.
4. `error_code` заполнять только при ошибке.

**Результат шага:** появляется полная техническая история LLM-вызовов.

### Шаг 7. Реализовать запись LLM-сообщения в `messages` при success
1. Добавить `INSERT` в `messages` только для успешной генерации.
2. Поля:
   - `chat_id`
   - `sender_id = llm_sender_id` из конфига
   - `author_type='llm'`
   - `text_content`
   - `sent_at = CURRENT_TIMESTAMP`
3. Не писать `llm_owner_id` (его не существует в контракте).

**Результат шага:** участники private-чата видят ответ ассистента как обычное сообщение чата.

### Шаг 8. Интеграционная ручная проверка
1. Тимур сохраняет user-сообщение и вызывает `LlmService` при `private + llm_enabled=true`.
2. Проверка БД:
   - есть запись в `llm_requests`;
   - при успехе есть новая запись в `messages` с `author_type='llm'` и `sender_id=llm_sender_id`.
3. При ошибке провайдера:
   - в `llm_requests` `status='error'` + `error_code`;
   - в `messages` новая запись не создаётся.

**Результат шага:** MVP-вертикальный срез считается реализованным.

---

## 7) Итоговый runtime-сценарий
1. Backend core Тимура сохраняет user-сообщение в `messages`.
2. Backend core проверяет `chats.llm_enabled`.
3. Если `chat_type='private'` и `llm_enabled=true`, backend вызывает `LlmService`.
4. `LlmService` собирает контекст из `messages` по `chat_id`.
5. `OllamaProvider` вызывает endpoint из `LlmConfig`.
6. После ответа `LlmService` пишет запись в `llm_requests`.
7. При `success` `LlmService` пишет LLM-ответ в `messages` как `author_type='llm'`.
8. При `error` `LlmService` пишет только `llm_requests` со `status='error'` и `error_code`.

---

## 8) Критерии готовности
- Все LLM-параметры централизованы в `LlmConfig`.
- `OllamaProvider`/`LlmService` не зависят от хардкода endpoint/model/timeout и т.д.
- SQL использует только актуальные имена полей (`chat_type`, `text_content`, `sent_at`, `response_ms`).
- Проект собирается после изменений.
- Ручная проверка: после вызова `LlmService` есть запись в `llm_requests`, а при success есть сообщение `author_type='llm'` в `messages` с `sender_id=llm_sender_id`.
