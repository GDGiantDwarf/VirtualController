#ifndef MULTICONTROLLERMANAGER_H
#define MULTICONTROLLERMANAGER_H

#include <QObject>
#include <QVector>
#include <vector>  // Pour std::vector
#include <memory>
#include <windows.h>
#include <ViGEm/Client.h>
#include "IInputSource.h"

/**
 * @brief Représente un seul contrôleur virtuel avec son état
 */
class VirtualController {
public:
    PVIGEM_TARGET target = nullptr;
    XUSB_REPORT report;
    int controllerId;
    bool connected = false;
    
    VirtualController(int id) : controllerId(id) {
        std::memset(&report, 0, sizeof(report));
    }
};

/**
 * @brief Gestionnaire de multiples contrôleurs virtuels (jusqu'à 4)
 * 
 * Cette classe gère jusqu'à 4 contrôleurs Xbox 360 virtuels via ViGEm.
 * Elle implémente une logique de retry pour la connexion et fournit
 * une interface simple pour mettre à jour l'état de chaque contrôleur.
 */
class MultiControllerManager : public QObject {
    Q_OBJECT
    
public:
    static constexpr int MAX_CONTROLLERS = 4;
    static constexpr int MAX_RETRY_ATTEMPTS = 3;
    static constexpr int RETRY_DELAY_MS = 1000;
    
    explicit MultiControllerManager(QObject* parent = nullptr);
    ~MultiControllerManager();
    
    /**
     * @brief Initialise le client ViGEm avec retry logic
     * @return true si l'initialisation réussit
     */
    bool initialize();
    
    /**
     * @brief Ajoute un nouveau contrôleur virtuel
     * @param controllerId ID du contrôleur (1-4)
     * @return true si ajouté avec succès
     */
    bool addController(int controllerId);
    
    /**
     * @brief Retire un contrôleur virtuel
     * @param controllerId ID du contrôleur à retirer
     */
    void removeController(int controllerId);
    
    /**
     * @brief Met à jour l'état d'un contrôleur spécifique
     * @param controllerId ID du contrôleur
     * @param state Nouvel état du contrôleur
     */
    void updateController(int controllerId, const ControllerState& state);
    
    /**
     * @brief Nettoie tous les contrôleurs et déconnecte
     */
    void cleanup();
    
    /**
     * @brief Vérifie si un contrôleur est connecté
     */
    bool isControllerConnected(int controllerId) const;
    
    /**
     * @brief Retourne le nombre de contrôleurs actifs
     */
    int getActiveControllerCount() const;
    
    /**
     * @brief Vérifie si le client ViGEm est connecté
     */
    bool isClientConnected() const { return m_clientConnected; }
    
signals:
    void controllerAdded(int controllerId);
    void controllerRemoved(int controllerId);
    void errorOccurred(const QString& error);
    void clientConnectionChanged(bool connected);
    
private:
    bool initializeClientWithRetry();
    bool connectToViGEm();
    XUSB_REPORT convertStateToReport(const ControllerState& state);
    VirtualController* getController(int controllerId);
    
    PVIGEM_CLIENT m_client;
    std::vector<std::unique_ptr<VirtualController>> m_controllers;  // CHANGÉ: std::vector au lieu de QVector
    bool m_clientConnected;
    int m_retryCount;
};

#endif // MULTICONTROLLERMANAGER_H