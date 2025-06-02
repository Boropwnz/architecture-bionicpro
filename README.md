# Описание решения.

## 1. Реализация PKCE.

В панели уравления Keycloak включена опция PKCE в режиме `S256`.

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
- Keycloak будет делать CORS чек
- Фронтенд будет делать GET на эндпоинт /reports

Исходящие запросы будут такие:
- Ответ на CORS чек с правильными заголовками
- Запрос в Keycloak на валидацию токена через token/introspect с правильными заголовками и Bearer токеном
- Ответ на GET /reports в виде специального заголовка Reports для простоты

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
