#ifndef CONTROLLERTAB_H
#define CONTROLLERTAB_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include "VirtualControllerWindow.h"
#include "VirtualGamepadManager.h"

class ControllerTab : public QWidget {
    Q_OBJECT
    
public:
    explicit ControllerTab(QWidget* parent = nullptr);
    ~ControllerTab();
    
private slots:
    void onOpenControllerClicked();
    void onControllerButtonPressed(const QString& buttonName);
    void onControllerButtonReleased(const QString& buttonName);
    void onGamepadConnectionChanged(bool connected);
    void onGamepadError(const QString& error);
    
private:
    void setupUI();
    XUSB_BUTTON mapButtonNameToXUSB(const QString& buttonName);
    
    QPushButton* openControllerButton;
    QLabel* statusLabel;
    QLabel* qrPlaceholder;
    QLabel* gamepadStatusLabel;
    VirtualControllerWindow* controllerWindow;
    VirtualGamepadManager* gamepadManager;
};

#endif // CONTROLLERTAB_H