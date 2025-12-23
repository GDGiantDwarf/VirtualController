#ifndef CONTROLLERTAB_H
#define CONTROLLERTAB_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QVector>
#include <vector>  // Pour std::vector
#include <memory>
#include "IInputSource.h"
#include "LocalInputSource.h"
#include "MultiControllerManager.h"

/**
 * @brief Onglet de gestion des contrôleurs (jusqu'à 4 locaux)
 * 
 * Cette classe gère la création et la connexion de jusqu'à 4 contrôleurs
 * locaux simultanément. Elle initialise le MultiControllerManager au moment
 * où un contrôleur est ajouté, pas au lancement de l'application.
 */
class ControllerTab : public QWidget {
    Q_OBJECT
    
public:
    explicit ControllerTab(QWidget* parent = nullptr);
    ~ControllerTab();
    
private slots:
    void onAddControllerClicked();
    void onRemoveControllerClicked(int controllerId);
    void onInputStateChanged(const ControllerState& state);
    void onInputConnectionChanged(bool connected);
    void onManagerConnectionChanged(bool connected);
    void onManagerError(const QString& error);
    void onControllerAdded(int controllerId);
    void onControllerRemoved(int controllerId);
    
private:
    void setupUI();
    void initializeManagerIfNeeded();
    void updateControllerStatus();
    QPushButton* createControllerButton(int controllerId);
    
    std::unique_ptr<MultiControllerManager> m_manager;
    std::vector<std::unique_ptr<IInputSource>> m_inputSources;  // CHANGÉ: std::vector au lieu de QVector
    
    QLabel* statusLabel;
    QLabel* managerStatusLabel;
    QPushButton* addControllerButton;
    QWidget* controllerButtonsWidget;
    QVBoxLayout* controllerButtonsLayout;
    QVector<QPushButton*> removeButtons;
    
    bool m_managerInitialized;
    int m_activeControllerCount;
};

#endif // CONTROLLERTAB_H