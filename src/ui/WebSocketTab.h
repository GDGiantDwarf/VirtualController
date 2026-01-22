#ifndef WEBSOCKETTAB_H
#define WEBSOCKETTAB_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QScrollArea>
#include <vector>
#include <memory>
#include "IInputSource.h"
#include "WebSocketInputSource.h"
#include "MultiControllerManager.h"
#include "QrCodeGenerator.h"

/**
 * @brief Structure pour gérer un contrôleur WebSocket
 */
struct WebSocketControllerInfo {
    int controllerId;
    std::unique_ptr<WebSocketInputSource> inputSource;
    QLabel* statusLabel;
    QPushButton* removeButton;
    QLabel* qrCodeLabel;
    
    WebSocketControllerInfo(int id) : controllerId(id), statusLabel(nullptr), 
                                       removeButton(nullptr), qrCodeLabel(nullptr) {}
};

/**
 * @brief Onglet de gestion des contrôleurs WebSocket (smartphone)
 * 
 * Cette classe permet de :
 * - Créer jusqu'à 4 serveurs WebSocket (un par contrôleur)
 * - Afficher des QR codes pour le pairing automatique
 * - Recevoir les inputs des smartphones et les transmettre à ViGEm
 */
class WebSocketTab : public QWidget {
    Q_OBJECT
    
public:
    explicit WebSocketTab(QWidget* parent = nullptr);
    ~WebSocketTab();
    
private slots:
    void onAddControllerClicked();
    void onRemoveControllerClicked(int controllerId);
    void onInputStateChanged(const ControllerState& state);
    void onInputConnectionChanged(bool connected);
    void onInputError(const QString& error);
    void onManagerConnectionChanged(bool connected);
    void onManagerError(const QString& error);
    void onControllerAdded(int controllerId);
    void onControllerRemoved(int controllerId);
    
private:
    void setupUI();
    void initializeManagerIfNeeded();
    void updateControllerStatus();
    QWidget* createControllerWidget(int controllerId, WebSocketInputSource* source);
    QImage generateQrCode(const QString& url);
    
    std::unique_ptr<MultiControllerManager> m_manager;
    std::vector<WebSocketControllerInfo> m_controllers;
    QrCodeGenerator m_qrGenerator;
    
    QLabel* statusLabel;
    QLabel* managerStatusLabel;
    QPushButton* addControllerButton;
    QVBoxLayout* controllersLayout;
    QScrollArea* scrollArea;
    QWidget* scrollContent;
    
    bool m_managerInitialized;
    int m_activeControllerCount;
    quint16 m_basePort; // Port de départ : 8765
};

#endif // WEBSOCKETTAB_H
