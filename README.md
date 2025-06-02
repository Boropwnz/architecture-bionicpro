# Описание решения.

## 1. Реализация PKCE.

В панели управления Keycloak включена опция PKCE в режиме `S256`.

В `keycloak/realm-export.json` добавлена секция:

```
"attributes": {
  "pkce.code.challenge.method": "S256"
}
```

В `frontend/src/App.tsx` добавлен параметр `initOptions`:
```
ReactKeycloakProvider authClient={keycloak} initOptions={{pkceMethod: 'S256'}}
```

## 2. Реализация backend api.

Я разработчик на Qt/C++, поэтому решил попрактиковаться в Qt фреймворке несмотря на сложности.

QNetworkAccessManager принимает и отправляет запросы.

Входящие запросы будут такие:
- Фронтенд будет делать GET на эндпоинт /reports, а мы слушать
  ```
  m_server.route("/reports", QHttpServerRequest::Method::AnyKnown, [this](const QHttpServerRequest &request) {
    return handleRequest(request);
  });
  if (!m_server.listen(QHostAddress::Any, 8000)) {
    qCritical() << "Failed to start server on port 8000";
  }
  ```
- Keycloak будет делать CORS чек, для простоты проверки CORS атрибутов пропущены
  ```
  // CORS check
  if (request.method() != QHttpServerRequest::Method::Get) {
      qInfo() << "Not get method";
      QHttpServerResponse response("CORS check reply", QHttpServerResponder::StatusCode::Ok);
      // Headers can be taken from request with request.headers() for complexity;
      response.setHeaders(headers);
      return response;
  }
  ```

Исходящие запросы будут такие:
- Ответ на CORS чек с правильными заголовками, для простоты они константные
  ```
  QHttpServerResponder::HeaderList headers = {
      {"Access-Control-Allow-Origin", "http://localhost:3000"},
      {"Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS"},
      {"Access-Control-Allow-Headers", "Content-Type, Authorization"},
      {"Access-Control-Allow-Credentials", "true"}
  };

  // CORS check
  if (request.method() != QHttpServerRequest::Method::Get) {
      qInfo() << "Not get method";
      QHttpServerResponse response("CORS check reply", QHttpServerResponder::StatusCode::Ok);
      // Headers can be taken from request with request.headers() for complexity;
      response.setHeaders(headers);
      return response;
  }
  ```
- Запрос в Keycloak на валидацию токена через token/introspect с правильными заголовками и Bearer токеном
  ```
  QNetworkRequest request;
  QString introspectionUrl = QString("%1/realms/%2/protocol/openid-connect/token/introspect").arg(m_keycloakUrl, m_realm);
  request.setUrl(QUrl(introspectionUrl));
  request.setRawHeader("Access-Control-Allow-Origin", "http://localhost:3000");
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
  QString authHeader = QString("%1:%2").arg(m_clientId, m_clientSecret);
  request.setRawHeader("Authorization", "Basic " + authHeader.toUtf8().toBase64());
  QByteArray postData;
  postData.append("token=" + token.toUtf8());
  postData.append("&client_id=" + m_clientId.toUtf8());
  postData.append("&client_secret=" + m_clientSecret.toUtf8());
  QNetworkReply *reply = m_networkManager->post(request, postData);
  ```
- Ответ на GET /reports в виде специального заголовка Reports вместо отчетов для простоты
  ```
  QHttpServerResponse response(message, code);
  response.setHeaders(headers);
  if (sendData) {
      response.addHeader("Reports", "Reports data");
  }
  return response;
  ```

На локальной машине все работает, но есть некоторые сложности с отладкой взаимодействия сервисов в docker desktop.

Для валидации токена необходимо чтобы адресат рапроса в отправителе совпадал с адресом получателя, для этого добавлено в `docker-compose.yaml`:

для keycloak
```
  KC_HOSTNAME: "localhost"
  KC_HOSTNAME_STRICT: "false"
```
для backend api
```
  extra_hosts:
    - "keycloak:host-gateway"
```

Для простоты, адреса, порты, айди сервисов и секрет представлены константами в бекенде. Они могут зачитываться из переменных окружения.

Дополнительные проверки реалмов, отправителя, получателя и прочие атрибуты запросов опустим. Конечно это можно добавить для должной безопасности.

Если приходит GET, будем извлекать токен. Если токена нет, возвращаем код `Unauthorized`.

Если токен есть, будем валидировать запросом на introspect эндпоинт в keyckoak. Если токен невалиден, возвращаем код `Unauthorized`.

Если токен валиден, проверим роль пользователя. Если она не "prothetic_user", возвращаем код `Forbidden`.

Если роль "prothetic_user", возвращаем код `Ok`. Добавим в ответ заголовок "Reports" имитирующий возвращение отчета.

Настройка keycloak производилась в панели администратора, здесь экспор настроек реалма: [realm-export-used-for-screenshots.json](https://github.com/Boropwnz/architecture-bionicpro/blob/sprint_8/keycloak/realm-export-used-for-screenshots.json)

## 3. Проверка работы.

Проверять будем логинясь разными пользователями и проверяя статус запросов в `developer tools` и логи контейнера с api.

Ниже привожу скриншоты результатов.

### Сборка:
![Сборка](https://github.com/Boropwnz/architecture-bionicpro/blob/sprint_8/01-build.png)

### Запуск:
![Запуск](https://github.com/Boropwnz/architecture-bionicpro/blob/sprint_8/02-up.png)

### Ошибочный логин, keycloak работает:
![Логин](https://github.com/Boropwnz/architecture-bionicpro/blob/sprint_8/03-login-fail.png)

### Отладка, валидация токена проходит на локальной машине:
![Отладка](https://github.com/Boropwnz/architecture-bionicpro/blob/sprint_8/04_verify_passed.png)

### Отчеты получены, успешный запрос на /reports, токен валиден (user: prothetic1, role: prothetic_user):
![Отчеты](https://github.com/Boropwnz/architecture-bionicpro/blob/sprint_8/05_login_prothetic-1.png)

### Отчеты получены, лог из контейнера с backend api:
![Отчеты, лог](https://github.com/Boropwnz/architecture-bionicpro/blob/sprint_8/05_login_prothetic-2.png)

### Пользователю запрещен доступ к отчетам, токен валиден  (user: user1, role: user):
![Нет прав](https://github.com/Boropwnz/architecture-bionicpro/blob/sprint_8/05_login_user-1.png)

### Пользователю запрещен доступ к отчетам, лог из контейнера с backend api:
![Нет прав, лог](https://github.com/Boropwnz/architecture-bionicpro/blob/sprint_8/05_login_user-2.png)

### Токен не валиден, неправильный секрет (или любая другая ошибка проверки токена, user: user1, role: user):
![Токен](https://github.com/Boropwnz/architecture-bionicpro/blob/sprint_8/05_login_user-1-inv.png)

### Токен не валиден, лог из контейнера с backend api:
![Токен, лог](https://github.com/Boropwnz/architecture-bionicpro/blob/sprint_8/05_login_user-2-inv.png)
