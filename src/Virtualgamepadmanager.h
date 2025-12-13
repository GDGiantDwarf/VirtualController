#ifndef VIRTUALGAMEPADMANAGER_H
#define VIRTUALGAMEPADMANAGER_H

#include <QObject>
#include <memory>

// IMPORTANT: windows.h must be included BEFORE ViGEm headers
#include <windows.h>
#include <ViGEm/Client.h>

class VirtualGamepadManager : public QObject {
    Q_OBJECT
    
public:
    explicit VirtualGamepadManager(QObject* parent = nullptr);
    ~VirtualGamepadManager();
    
    bool initialize();
    void cleanup();
    
    // Button methods
    void pressButton(XUSB_BUTTON button);
    void releaseButton(XUSB_BUTTON button);
    void releaseAllButtons();
    
    // D-Pad methods
    void setDPad(bool up, bool down, bool left, bool right);
    
    // Analog stick methods (values: -32768 to 32767)
    void setLeftStick(short x, short y);
    void setRightStick(short x, short y);
    
    // Trigger methods (values: 0 to 255)
    void setTriggers(unsigned char left, unsigned char right);
    
    bool isConnected() const { return m_connected; }
    
signals:
    void connectionStatusChanged(bool connected);
    void errorOccurred(const QString& error);
    
private:
    void updateController();
    
    PVIGEM_CLIENT m_client;
    PVIGEM_TARGET m_controller;
    XUSB_REPORT m_report;
    bool m_connected;
};

#endif // VIRTUALGAMEPADMANAGER_H