#ifndef IINPUTSOURCE_H
#define IINPUTSOURCE_H

#include <QObject>
#include <QString>

/**
 * @brief Structure représentant l'état complet d'un contrôleur
 * 
 * Cette structure est utilisée pour transmettre l'état d'un contrôleur
 * de manière unifiée, que ce soit depuis une UI de test locale ou
 * depuis un smartphone connecté via réseau.
 */
struct ControllerState {
    // Buttons (bitmask compatible XUSB_BUTTON)
    unsigned short buttons = 0;
    
    // D-Pad
    bool dpadUp = false;
    bool dpadDown = false;
    bool dpadLeft = false;
    bool dpadRight = false;
    
    // Analog sticks (range: -32768 to 32767)
    short leftStickX = 0;
    short leftStickY = 0;
    short rightStickX = 0;
    short rightStickY = 0;
    
    // Triggers (range: 0 to 255)
    unsigned char leftTrigger = 0;
    unsigned char rightTrigger = 0;
    
    // Controller ID (1-4 for local multiplayer)
    int controllerId = 1;
    
    /**
     * @brief Réinitialise l'état du contrôleur
     */
    void reset() {
        buttons = 0;
        dpadUp = dpadDown = dpadLeft = dpadRight = false;
        leftStickX = leftStickY = rightStickX = rightStickY = 0;
        leftTrigger = rightTrigger = 0;
    }
};

/**
 * @brief Interface abstraite pour toutes les sources d'input
 * 
 * Cette interface permet de traiter de manière unifiée les inputs
 * provenant de différentes sources (UI de test, smartphone, clavier, etc.)
 */
class IInputSource : public QObject {
    Q_OBJECT
    
public:
    explicit IInputSource(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~IInputSource() = default;
    
    /**
     * @brief Démarre la source d'input
     * @return true si démarré avec succès
     */
    virtual bool start() = 0;
    
    /**
     * @brief Arrête la source d'input
     */
    virtual void stop() = 0;
    
    /**
     * @brief Retourne si la source est active
     */
    virtual bool isActive() const = 0;
    
    /**
     * @brief Retourne l'état actuel du contrôleur
     */
    virtual ControllerState getState() const = 0;
    
signals:
    /**
     * @brief Émis quand l'état du contrôleur change
     */
    void stateChanged(const ControllerState& state);
    
    /**
     * @brief Émis en cas d'erreur
     */
    void errorOccurred(const QString& error);
    
    /**
     * @brief Émis quand le statut de connexion change
     */
    void connectionStatusChanged(bool connected);
};

#endif // IINPUTSOURCE_H
