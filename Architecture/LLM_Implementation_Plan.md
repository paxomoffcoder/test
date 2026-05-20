# План реализации LLM-интеграции — MVP (обновлённый порядок)

## 0) Что делаем сейчас
На первом шаге делаем **минимальный конфигурационный слой**, и только после него переходим к вертикальному срезу:

1) `LlmConfig` + `LlmConfigManager` (минимум)
2) `ILLMProvider → OllamaProvider → ContextBuilder → LlmService`
3) запись в `llm_requests` и `messages`

Без изменения схемы БД, без runtime config update, без retry endpoint, без Dashboard/аналитики, без переписывания HTTP-роутинга и сессий.

---

## 1) Ограничения MVP (зафиксировано)
1. Используем текущую схему БД как есть.
2. В `llm_requests` используем поле **`response_ms`** (не `duration_ms`).
3. Для `llm_requests.status` в MVP только:
   - `success`
   - `error`
4. Техническую причину ошибки пишем в `llm_requests.error_code`.
5. Очередь запросов живёт **внутри `LlmService`**.
6. Проверки сессии, HTTP endpoint, сохранение user-сообщения, `llm_enabled` и доставка клиенту — зона Тимура.
7. Пока не используем `std::atomic<std::shared_ptr<...>>` (ориентир на C++17, runtime update будет позже).

---

## 2) Минимальный конфигурационный слой (сделать ПЕРВЫМ)

### 2.1 Структура `LlmConfig`
Нужна единая структура с дефолтами:
- `endpoint = "http://localhost:11434"`
- `model = "qwen2.5:4b"`
- `timeout_ms = 180000`
- `temperature = 0.7`
- `max_tokens = 1024`
- `context_window = 20`
- `streaming = false`

### 2.2 Минимальный `LlmConfigManager`
На текущем шаге менеджер должен:
- создавать дефолтный `LlmConfig`;
- отдавать текущую конфигурацию через `current()`;
- выполнять простую валидацию диапазонов;
- **не** поддерживать runtime update;
- **не** читать `server_config`.

### 2.3 Валидация (минимум)
Проверять только базовые диапазоны:
- `timeout_ms > 0`
- `temperature` в разумном диапазоне (например, `0.0..1.0`)
- `max_tokens > 0`
- `context_window > 0`
- `endpoint/model` не пустые

### 2.4 Требование к интеграции
`OllamaProvider` и `LlmService` обязаны брать параметры из `LlmConfig`, а не хардкодить:
- endpoint
- model
- timeout
- temperature
- max_tokens
- context_window

**Критерий готовности слоя:** все LLM-параметры находятся в одном месте, и для будущего подключения `server_config` не нужно переписывать `OllamaProvider`/`LlmService`.

---

## 3) Зона ответственности
### Тимур (backend платформенный слой)
- HTTP endpoint
- проверка сессии
- сохранение user-сообщения
- проверка `llm_enabled`
- вызов `LlmService`
- доставка новых сообщений клиенту

### Ты (LLM-модуль)
- `LlmConfig` + `LlmConfigManager` (минимальный)
- `LlmService`
- очередь LLM-запросов (внутри `LlmService`)
- `ContextBuilder`
- `OllamaProvider`
- запись в `llm_requests`
- запись LLM-ответа в `messages`

---

## 4) Пошаговая реализация вертикального среза (после конфига)

### Шаг 1. Контракт провайдера: `ILLMProvider`
**Что сделать:**
- Ввести интерфейс `ILLMProvider` с методом генерации:
  - вход: массив контекста `[{role, content}]` + параметры из `LlmConfig`
  - выход: `text`, `model`, `response_ms`, `ok`, `error_code`

**Критерий готовности:**
- `LlmService` зависит только от `ILLMProvider`, а не от конкретной реализации.

---

### Шаг 2. Реализация `OllamaProvider`
**Что сделать:**
- Реализовать вызов OpenAI-compatible `/v1/chat/completions`.
- Брать `endpoint/model/timeout/temperature/max_tokens/streaming` из `LlmConfig`.
- Мерить время ответа и возвращать `response_ms`.
- Нормализовать ошибки в `error_code` (`timeout`, `network_error`, `invalid_json`, `provider_5xx`).

**Критерий готовности:**
- Изолированный вызов к Ollama возвращает либо текст ответа, либо нормализованную ошибку.

---

### Шаг 3. `ContextBuilder`
**Что сделать:**
- Читать последние N сообщений из `messages` по `chat_id`.
- Брать `N` из `config.context_window`.
- Формировать роли для LLM (`system`, `user`, `assistant`) на основе `author_type`.
- Поддержать только `private` и `llm_chat` (валидация типа чата остаётся в платформенном слое у Тимура).

**Критерий готовности:**
- Для заданного `chat_id` строится стабильный, предсказуемый контекст для LLM.

---

### Шаг 4. `LlmService` + внутренняя очередь
**Что сделать:**
- Реализовать `LlmService::enqueue(chatId, userId, userPrompt)`.
- Внутри сервиса держать FIFO-очередь и 1 активную задачу (MVP).
- Выполнять задачу асинхронно, чтобы не блокировать вызывающий поток.
- Использовать текущий `LlmConfig` для вызова провайдера и лимитов контекста.

**Критерий готовности:**
- Несколько вызовов `enqueue` обрабатываются последовательно и предсказуемо.

---

### Шаг 5. Запись в БД (`llm_requests` и `messages`)
**Что сделать:**
1. После завершения генерации писать запись в `llm_requests`:
   - `chat_id`
   - `user_id`
   - `prompt`
   - `response`
   - `model`
   - `response_ms`
   - `status` = `success` или `error`
   - `error_code` (только если `error`)
   - `created_at`
2. При `success` писать LLM-сообщение в `messages`:
   - `chat_id`
   - `sender_id` (по вашей текущей договорённости/схеме)
   - `content` = текст LLM
   - `author_type` = `'llm'`
   - `llm_owner_id` = `userId`
   - `created_at`

**Критерий готовности:**
- После успешной генерации в БД есть и запись запроса, и сообщение LLM.

---

## 5) Что НЕ делаем на этом шаге
- Runtime config update.
- Чтение LLM-настроек из `server_config`.
- Retry endpoint.
- Новые статусы `queued/processing/timeout/canceled`.
- Любые изменения схемы БД.
- Любые задачи по Dashboard и аналитике.
- Переписывание backend HTTP-роутинга, сессий и клиентской доставки.

---

## 6) Проверка готовности (ручной acceptance)
1. Проект после изменений собирается.
2. Все параметры LLM находятся в одном месте (`LlmConfig`/`LlmConfigManager`).
3. Нужно вручную вызвать `LlmService` для `private` или `llm_chat` и убедиться, что:
   - есть строка в `llm_requests`;
   - есть LLM-сообщение в `messages` с `author_type='llm'` и `llm_owner_id=userId`.

Это и есть целевой MVP-вертикальный срез для текущего этапа.
