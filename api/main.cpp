#include <QCoreApplication>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QHttpServerResponder>

class ReportsBackendApi : public QObject {
    Q_OBJECT

public:
    explicit ReportsBackendApi(QObject *parent = nullptr) : QObject(parent), m_networkManager(new QNetworkAccessManager(this)) {
        m_server.route("/reports", QHttpServerRequest::Method::AnyKnown, [this](const QHttpServerRequest &request) {
            return handleRequest(request);
        });

        if (!m_server.listen(QHostAddress::Any, 8000)) {
            qCritical() << "Failed to start server on port 8000";
        }
        else {
            qInfo() << "Server started on http://localhost:8000";
        }
    }

private:
    QHttpServer m_server;
    QNetworkAccessManager *m_networkManager;

    const QString m_keycloakUrl = "http://localhost:8080";
    const QString m_realm = "reports-realm";
    const QString m_clientId = "reports-api";
    const QString m_clientSecret = "oNwoLQdvJAvRcL89SydqCWCe5ry1jMgq";

    QHttpServerResponse handleRequest(const QHttpServerRequest &request) {
        qInfo() << "Got request";
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
        // Get reports
        else {
            qInfo() << "Get method";
            const QString token = getBearerToken(request);
            QString message;
            bool sendData(false);
            QHttpServerResponder::StatusCode code;
            if (token.isEmpty()) {
                message = "Authorization header missing";
                code = QHttpServerResponder::StatusCode::Unauthorized;
            }
            else {
                const auto [isValid, roles] = verifyToken(token);
                if (!isValid) {
                    qInfo() << "Invalid token";
                    message = "Invalid token";
                    code = QHttpServerResponder::StatusCode::Unauthorized;
                }
                else {
                    if (!roles.contains("prothetic_user")) {
                        message = "Access denied: Prothetic user role required";
                        code = QHttpServerResponder::StatusCode::Forbidden;
                    }
                    else {
                        qInfo() << "Reports sent!";
                        message = "Prothetic user reports sent!";
                        code = QHttpServerResponder::StatusCode::Ok;
                        sendData = true;
                    }
                }
            }
            QHttpServerResponse response(message, code);
            response.setHeaders(headers);
            if (sendData) {
                response.addHeader("Reports", "Reports data");
            }
            return response;
        }
    }

    QString getBearerToken(const QHttpServerRequest &request) const {
        auto authHeader = request.value("Authorization");
        if (authHeader.startsWith("Bearer ")) {
            return authHeader.mid(7).trimmed();
        }
        else {
            return "";
        }    
    }

    std::pair<bool, QStringList> verifyToken(const QString &token) {
        qInfo() << "Verify token";
        QEventLoop loop;
        QNetworkRequest request;
        QString internalKeycloakUrl = "http://keycloak:8080";
        QString introspectionUrl = QString("%1/realms/%2/protocol/openid-connect/token/introspect").arg(internalKeycloakUrl, m_realm);
        //QString introspectionUrl = QString("%1/realms/%2/protocol/openid-connect/token/introspect").arg(m_keycloakUrl, m_realm);
        request.setUrl(QUrl(introspectionUrl));
        // request.setRawHeader("Access-Control-Allow-Origin", "http://localhost:3000");
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        QByteArray postData;
        postData.append("token=" + token.toUtf8());
        postData.append("&client_id=" + m_clientId.toUtf8());
        postData.append("&client_secret=" + m_clientSecret.toUtf8());
        QNetworkReply *reply = m_networkManager->post(request, postData);
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "Keycloak error:" << reply->errorString();
            return {false, {}};
        }
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        const QJsonObject response = doc.object();
        const bool isActive = response["active"].toBool();
        QStringList roles;
        if (isActive) {
            const QJsonObject realmAccess = response["realm_access"].toObject();
            const QJsonArray rolesArray = realmAccess["roles"].toArray();
            for (const auto &role : rolesArray) {
                roles.append(role.toString());
            }
        }
        reply->deleteLater();
        return {isActive, roles};
    }
};

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);
    ReportsBackendApi server;
    return a.exec();
}

#include "main.moc"
