#ifndef WEBSOCKETINPUTSOURCE_H
#define WEBSOCKETINPUTSOURCE_H

#include "IInputSource.h"
#include <QWebSocketServer>
#include <QWebSocket>
#include <QMap>
#include <QHostAddress>

/**
 * @brief Source d'input via WebSocket pour contrôleurs mobiles
 * 
 * Cette classe implémente IInputSource et permet de recevoir des inputs
 * depuis des applications mobiles connectées via WebSocket.
 * Supporte jusqu'à 4 contrôleurs simultanés.
 */
class WebSocketInputSource : public IInputSource {
    Q_OBJECT
    
public:
    explicit WebSocketInputSource(int controllerId = 1, QObject* parent = nullptr);
    ~WebSocketInputSource() override;
    
    // Interface IInputSource
    bool start() override;
    void stop() override;
    bool isActive() const override;
    ControllerState getState() const override;
    
    /**
     * @brief Retourne l'URL de connexion WebSocket
     */
    QString getConnectionUrl() const;
    
    /**
     * @brief Retourne l'adresse IP locale
     */
    QString getLocalIp() const;
    
    /**
     * @brief Retourne le port du serveur
     */
    quint16 getPort() const { return m_port; }
    
    /**
     * @brief Retourne le nombre de clients connectés
     */
    int getConnectedClients() const { return m_clients.size(); }
    
private slots:
    void onNewConnection();
    void onTextMessageReceived(const QString& message);
    void onClientDisconnected();
    
private:
    void processButtonEvent(const QJsonObject& data);
    void processAnalogEvent(const QJsonObject& data);
    void updateButtonBitmask(const QString& button, bool pressed);
    short normalizeAnalog(double value);
    
    QWebSocketServer* m_server;
    QList<QWebSocket*> m_clients;
    ControllerState m_currentState;
    quint16 m_port;
    bool m_isActive;
    
    // Mapping des boutons vers le bitmask XUSB
    static const QMap<QString, unsigned short> ButtonMap;

    void DebugPrintReceivedMessage(const QString& message);
};

#endif // WEBSOCKETINPUTSOURCE_H
