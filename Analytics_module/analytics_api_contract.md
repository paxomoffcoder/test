# Analytics API Contract (for future server integration)

## Scope
This document defines the **future HTTP contract** for analytics integration.

Important:
- `DataCollector` remains an isolated, read-only module.
- `DataCollector` does **not** depend on HTTP.
- No new metrics are introduced beyond the agreed architecture.
- Forbidden data sources are not used: `files`, `event_logs`, `chats`, `read_state`, `chat_members`.

## Common query parameters

### `period`
Allowed values:
- `day`
- `week`
- `month`

Mapping to enum:
- `day` -> `DataCollector::Period::Day`
- `week` -> `DataCollector::Period::Week`
- `month` -> `DataCollector::Period::Month`

### `group_by` (only for dynamic endpoints)
Allowed values:
- `hour`
- `day`
- `week`

Mapping to enum:
- `hour` -> `DataCollector::Granularity::Hour`
- `day` -> `DataCollector::Granularity::Day`
- `week` -> `DataCollector::Granularity::Week`

---

## 1) GET `/api/stats/summary`

### Purpose
Return compact summary for selected period.

### Query parameters
- `period` (required): `day | week | month`

### DataCollector mapping
- `DataCollector::getSummary(period)`

### Response JSON
```json
{
  "active_users_total": 42,
  "messages_total": 1250,
  "llm_requests_total": 87
}
```

### Data notes
- Summary includes **only**:
  - `active_users_total`
  - `messages_total`
  - `llm_requests_total`

---

## 2) GET `/api/stats/users/active`

### Purpose
Return active users dynamics over time.

### Query parameters
- `period` (required): `day | week | month`
- `group_by` (required): `hour | day | week`

### DataCollector mapping
- `DataCollector::getActiveUsersDynamics(period, granularity)`

### Response JSON
```json
[
  {"bucket": "2026-05-01", "count": 15}
]
```

### Data notes
- Active users = distinct users who sent messages during period.
- Only user-authored messages are counted (`author_type = 'user'`).

---

## 3) GET `/api/stats/users/new`

### Purpose
Return number of new registrations during period.

### Query parameters
- `period` (required): `day | week | month`

### DataCollector mapping
- `DataCollector::getNewRegistrations(period)`

### Response JSON
```json
{
  "new_registrations": 8
}
```

### Data notes
- Source: `users.created_at`.

---

## 4) GET `/api/stats/messages/total`

### Purpose
Return total messages during period.

### Query parameters
- `period` (required): `day | week | month`

### DataCollector mapping
- `DataCollector::getMessagesTotal(period)`

### Response JSON
```json
{
  "messages_total": 1250
}
```

### Data notes
- Source: `messages.sent_at`.

---

## 5) GET `/api/stats/messages/dynamics`

### Purpose
Return messages dynamics over time.

### Query parameters
- `period` (required): `day | week | month`
- `group_by` (required): `hour | day | week`

### DataCollector mapping
- `DataCollector::getMessagesDynamics(period, granularity)`

### Response JSON
```json
[
  {"bucket": "2026-05-01", "count": 145}
]
```

### Data notes
- Source: `messages.sent_at`.

---

## 6) GET `/api/stats/llm/total`

### Purpose
Return total LLM requests during period.

### Query parameters
- `period` (required): `day | week | month`

### DataCollector mapping
- `DataCollector::getLLMRequestsTotal(period)`

### Response JSON
```json
{
  "llm_requests_total": 87
}
```

### Data notes
- Source: `llm_requests.created_at`.

---

## 7) GET `/api/stats/llm/dynamics`

### Purpose
Return LLM requests dynamics over time.

### Query parameters
- `period` (required): `day | week | month`
- `group_by` (required): `hour | day | week`

### DataCollector mapping
- `DataCollector::getLLMRequestsDynamics(period, granularity)`

### Response JSON
```json
[
  {"bucket": "2026-05-01", "count": 12}
]
```

### Data notes
- Source: `llm_requests.created_at`.

---

## 8) GET `/api/stats/llm/performance`

### Purpose
Return LLM performance metrics for period.

### Query parameters
- `period` (required): `day | week | month`

### DataCollector mapping
- `DataCollector::getLLMPerformance(period)`

### Response JSON
```json
{
  "llm_avg_response_ms": 1250,
  "llm_error_rate": 0.03
}
```

### Data notes
- `llm_avg_response_ms` is average response latency in milliseconds.
- `llm_error_rate` is a fraction in range `[0..1]`.

---

## Future Dashboard Client

A future Dashboard Client will be implemented as a **separate Qt application**.

Planned integration approach:
- Use `QNetworkAccessManager` to call the analytics endpoints listed above.
- Parse JSON responses into UI models/charts.
- Keep Dashboard concerns (auth, retries, caching, visualization) outside `DataCollector`.

Dashboard implementation is out of scope for this document.
