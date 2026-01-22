#include "WebSocketInputSource.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>
#include <QDebug>
#include <QtMath>
#include <iostream>

// Définition des constantes XUSB_BUTTON
#define XUSB_GAMEPAD_DPAD_UP            0x0001
#define XUSB_GAMEPAD_DPAD_DOWN          0x0002
#define XUSB_GAMEPAD_DPAD_LEFT          0x0004
#define XUSB_GAMEPAD_DPAD_RIGHT         0x0008
#define XUSB_GAMEPAD_START              0x0010
#define XUSB_GAMEPAD_BACK               0x0020
#define XUSB_GAMEPAD_LEFT_THUMB         0x0040
#define XUSB_GAMEPAD_RIGHT_THUMB        0x0080
#define XUSB_GAMEPAD_LEFT_SHOULDER      0x0100
#define XUSB_GAMEPAD_RIGHT_SHOULDER     0x0200
#define XUSB_GAMEPAD_A                  0x1000
#define XUSB_GAMEPAD_B                  0x2000
#define XUSB_GAMEPAD_X                  0x4000
#define XUSB_GAMEPAD_Y                  0x8000

// Mapping des boutons
const QMap<QString, unsigned short> WebSocketInputSource::ButtonMap = {
    {"A", XUSB_GAMEPAD_A},
    {"B", XUSB_GAMEPAD_B},
    {"X", XUSB_GAMEPAD_X},
    {"Y", XUSB_GAMEPAD_Y},
    {"LB", XUSB_GAMEPAD_LEFT_SHOULDER},
    {"RB", XUSB_GAMEPAD_RIGHT_SHOULDER},
    {"L3", XUSB_GAMEPAD_LEFT_THUMB},
    {"R3", XUSB_GAMEPAD_RIGHT_THUMB},
    {"START", XUSB_GAMEPAD_START},
    {"SELECT", XUSB_GAMEPAD_BACK},
    {"DPAD_UP", XUSB_GAMEPAD_DPAD_UP},
    {"DPAD_DOWN", XUSB_GAMEPAD_DPAD_DOWN},
    {"DPAD_LEFT", XUSB_GAMEPAD_DPAD_LEFT},
    {"DPAD_RIGHT", XUSB_GAMEPAD_DPAD_RIGHT}
};

WebSocketInputSource::WebSocketInputSource(int controllerId, QObject* parent)
    : IInputSource(parent)
    , m_server(nullptr)
    , m_port(8765)
    , m_isActive(false)
{
    m_currentState.controllerId = controllerId;
}

void WebSocketInputSource::DebugPrintReceivedMessage(const QString& message) {
    std::cout << "Received Message: " << message.toStdString() << std::endl;
}

WebSocketInputSource::~WebSocketInputSource() {
    stop();
}

bool WebSocketInputSource::start() {
    if (m_isActive) {
        return true;
    }
    
    m_server = new QWebSocketServer(
        QStringLiteral("VirtualController Server"),
        QWebSocketServer::NonSecureMode,
        this
    );
    
    if (!m_server->listen(QHostAddress::Any, m_port)) {
        qWarning() << "Impossible de démarrer le serveur WebSocket:" << m_server->errorString();
        emit errorOccurred("Impossible de démarrer le serveur: " + m_server->errorString());
        delete m_server;
        m_server = nullptr;
        return false;
    }
    
    connect(m_server, &QWebSocketServer::newConnection,
            this, &WebSocketInputSource::onNewConnection);
    
    m_isActive = true;
    
    qInfo() << "========================================";
    qInfo() << "Serveur WebSocket démarré";
    qInfo() << "========================================";
    qInfo() << "Adresse:" << getLocalIp() << ":" << m_port;
    qInfo() << "URL:" << getConnectionUrl();
    qInfo() << "En attente de connexions...";
    qInfo() << "";
    
    emit connectionStatusChanged(true);
    return true;
}

void WebSocketInputSource::stop() {
    if (!m_isActive) {
        return;
    }
    
    // Déconnecter tous les clients
    for (QWebSocket* client : m_clients) {
        client->close();
    }
    m_clients.clear();
    
    // Fermer le serveur
    if (m_server) {
        m_server->close();
        delete m_server;
        m_server = nullptr;
    }
    
    m_isActive = false;
    m_currentState.reset();
    
    qInfo() << "STOP Serveur WebSocket arrêté";
    emit connectionStatusChanged(false);
}

bool WebSocketInputSource::isActive() const {
    return m_isActive;
}

ControllerState WebSocketInputSource::getState() const {
    return m_currentState;
}

QString WebSocketInputSource::getConnectionUrl() const {
    return QString("ws://%1:%2/controller").arg(getLocalIp()).arg(m_port);
}

QString WebSocketInputSource::getLocalIp() const {
    // Obtenir la première adresse IPv4 non-localhost
    const QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    for (const QHostAddress& address : addresses) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && 
            address != QHostAddress::LocalHost) {
            return address.toString();
        }
    }
    return "localhost";
}

void WebSocketInputSource::onNewConnection() {
    QWebSocket* client = m_server->nextPendingConnection();
    
    qInfo() << "Contrôleur connecté depuis" 
            << client->peerAddress().toString() << ":" << client->peerPort();
    
    m_clients.append(client);
    
    connect(client, &QWebSocket::textMessageReceived,
            this, &WebSocketInputSource::onTextMessageReceived);
    connect(client, &QWebSocket::disconnected,
            this, &WebSocketInputSource::onClientDisconnected);
}

void WebSocketInputSource::onTextMessageReceived(const QString& message) {
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) {
        qWarning() << "Warning:  Message JSON invalide";
        return;
    }
    
    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();
    
    if (type == "button") {
        processButtonEvent(obj);
        std::cout << "Processed button event." << std::endl;
        DebugPrintReceivedMessage(message); //debug print, remove in production
    } else if (type == "analog") {
        processAnalogEvent(obj);
        std::cout << "Processed analog event." << std::endl;
        DebugPrintReceivedMessage(message); //debug print, remove in production
    } else {
        qDebug() << "Type de message inconnu:" << type;
    }
    
    // Émettre le changement d'état
    emit stateChanged(m_currentState);
}

void WebSocketInputSource::onClientDisconnected() {
    QWebSocket* client = qobject_cast<QWebSocket*>(sender());
    if (client) {
        qInfo() << "Contrôleur déconnecté depuis"
                << client->peerAddress().toString() << ":" << client->peerPort();
        
        m_clients.removeAll(client);
        client->deleteLater();
        
        // Réinitialiser l'état si plus de clients
        if (m_clients.isEmpty()) {
            m_currentState.reset();
            emit stateChanged(m_currentState);
        }
    }
}

void WebSocketInputSource::processButtonEvent(const QJsonObject& data) {
    QString button = data["button"].toString();
    bool pressed = data["pressed"].toBool();
    
    qDebug() << "Bouton:" << button << (pressed ? "PRESSED" : "RELEASED");
    
    updateButtonBitmask(button, pressed);
}

void WebSocketInputSource::processAnalogEvent(const QJsonObject& data) {
    QString stick = data["stick"].toString();
    double x = data["x"].toDouble();
    double y = data["y"].toDouble();
    
    qDebug() << QString("Analog %1: X=%2, Y=%3")
                .arg(stick, 5)
                .arg(x, 5, 'f', 2)
                .arg(y, 5, 'f', 2);
    
    if (stick == "left") {
        m_currentState.leftStickX = normalizeAnalog(x);
        m_currentState.leftStickY = normalizeAnalog(y); // in the future, add option for y inversion
    } else if (stick == "right") {
        m_currentState.rightStickX = normalizeAnalog(x);
        m_currentState.rightStickY = normalizeAnalog(y); // in the future, add option for y inversion
    }
}

void WebSocketInputSource::updateButtonBitmask(const QString& button, bool pressed) {
    if (ButtonMap.contains(button)) {
        unsigned short mask = ButtonMap[button];
        
        if (pressed) {
            m_currentState.buttons |= mask;
            
            // Mettre à jour les flags D-Pad
            if (button == "DPAD_UP") m_currentState.dpadUp = true;
            else if (button == "DPAD_DOWN") m_currentState.dpadDown = true;
            else if (button == "DPAD_LEFT") m_currentState.dpadLeft = true;
            else if (button == "DPAD_RIGHT") m_currentState.dpadRight = true;
        } else {
            m_currentState.buttons &= ~mask;
            
            // Mettre à jour les flags D-Pad
            if (button == "DPAD_UP") m_currentState.dpadUp = false;
            else if (button == "DPAD_DOWN") m_currentState.dpadDown = false;
            else if (button == "DPAD_LEFT") m_currentState.dpadLeft = false;
            else if (button == "DPAD_RIGHT") m_currentState.dpadRight = false;
        }
    }
}

short WebSocketInputSource::normalizeAnalog(double value) {
    // Convertir de [-1.0, 1.0] à [-32768, 32767];
    return static_cast<short>(qBound(-1.0, value, 1.0) * 32767.0);
}
