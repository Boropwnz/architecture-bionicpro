# Описание решения.

## 1. Реализация PKCE.

В панели уравления keycloak включена опция PKCE в режиме `S256`.

В `keycloak/realm-export.json` добавлена секция:

```
"attributes": {
  "pkce.code.challenge.method": "S256"
}
```


## 2. Реализация backend api.



## 3. Проверка работы.
