#ifndef LOCALINPUTSOURCE_H
#define LOCALINPUTSOURCE_H

#include "IInputSource.h"
#include "VirtualControllerWindow.h"
#include <QSet>
#include <memory>

/**
 * @brief Source d'input locale via une fenêtre de test
 * 
 * Cette classe implémente IInputSource pour fournir des inputs
 * depuis une fenêtre de contrôleur virtuel de test (UI locale).
 * Elle suit l'état de tous les boutons pressés pour gérer
 * correctement les diagonales et les pressions multiples.
 */
class LocalInputSource : public IInputSource {
    Q_OBJECT
    
public:
    explicit LocalInputSource(int controllerId, QObject* parent = nullptr);
    ~LocalInputSource() override;
    
    bool start() override;
    void stop() override;
    bool isActive() const override;
    ControllerState getState() const override;
    
    /**
     * @brief Retourne la fenêtre de contrôleur (peut être null)
     */
    VirtualControllerWindow* getWindow() const { return m_window.get(); }
    
    /**
     * @brief Définit la position du stick analogique
     */
    void setAnalogStick(short x, short y);
    
private slots:
    void onButtonPressed(const QString& buttonName);
    void onButtonReleased(const QString& buttonName);
    void onAnalogStickMoved(short x, short y);
    
private:
    void updateState();
    unsigned short mapButtonNameToXUSB(const QString& buttonName);
    
    std::unique_ptr<VirtualControllerWindow> m_window;
    ControllerState m_state;
    QSet<QString> m_pressedButtons;
    int m_controllerId;
    bool m_active;
};

#endif // LOCALINPUTSOURCE_H