#ifndef WEBSOCKETTAB_H
#define WEBSOCKETTAB_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <array>
#include <memory>
#include "IInputSource.h"
#include "WebSocketInputSource.h"
#include "MultiControllerManager.h"
#include "QrCodeGenerator.h"

/**
 * @brief Slot de contrôleur associant port, WebSocket et ViGEm
 */
struct ControllerSlot {
    quint16 port;                                      // Port number (8765-8768)
    std::unique_ptr<WebSocketInputSource> inputSource; // WebSocket server
    int vigEmControllerId;                             // ViGEm controller ID (-1 if not created)
    bool isConnected;                                  // Has active client connection
    
    ControllerSlot() : port(0), vigEmControllerId(-1), isConnected(false) {}
};

/**
 * @brief Onglet de gestion des contrôleurs WebSocket (smartphone)
 * 
 * Cette classe permet de :
 * - Créer 4 serveurs WebSocket toujours actifs (ports 8765-8768)
 * - Afficher un QR code unique pour le prochain port disponible
 * - Créer un contrôleur ViGEm seulement quand un client se connecte
 * - Recevoir les inputs des smartphones et les transmettre à ViGEm
 */
class WebSocketTab : public QWidget {
    Q_OBJECT
    
public:
    explicit WebSocketTab(QWidget* parent = nullptr);
    ~WebSocketTab();
    
private slots:
    void onInputStateChanged(const ControllerState& state);
    void onInputConnectionChanged(bool connected);
    void onInputError(const QString& error);
    void onManagerConnectionChanged(bool connected);
    void onManagerError(const QString& error);
    void onControllerAdded(int controllerId);
    void onControllerRemoved(int controllerId);
    
private:
    void setupUI();
    void initializeAllServers();
    void initializeManagerIfNeeded();
    void updateQrCode();
    void createVigEmControllerForSlot(int slotIndex);
    void removeVigEmControllerForSlot(int slotIndex);
    QImage generateQrCode(const QString& url);
    int getLowestAvailableSlotIndex() const;
    int getSlotIndexForInputSource(WebSocketInputSource* source) const;
    
    std::unique_ptr<MultiControllerManager> m_manager;
    std::array<ControllerSlot, 4> m_controllerSlots;
    QrCodeGenerator m_qrGenerator;
    
    QLabel* qrCodeLabel;
    QLabel* urlLabel;
    QLabel* managerStatusLabel;
    QLabel* statusLabel;
    
    bool m_managerInitialized;
    static constexpr int MAX_SERVERS = 4;
    quint16 m_basePort = 8770; // Ports 8770-8773
};

#endif // WEBSOCKETTAB_H
