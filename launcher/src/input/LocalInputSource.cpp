#include "LocalInputSource.h"
#include <QDebug>
#include <windows.h>
#include <ViGEm/Client.h>

LocalInputSource::LocalInputSource(int controllerId, QObject* parent)
    : IInputSource(parent)
    , m_controllerId(controllerId)
    , m_active(false)
{
    m_state.controllerId = controllerId;
}

LocalInputSource::~LocalInputSource() {
    stop();
}

bool LocalInputSource::start() {
    if (m_active) {
        qWarning() << "LocalInputSource" << m_controllerId << "already active";
        return true;
    }
    
    qDebug() << "Starting LocalInputSource for controller" << m_controllerId;
    
    // Create controller window
    m_window = std::make_unique<VirtualControllerWindow>(m_controllerId);
    
    // Connect signals
    connect(m_window.get(), &VirtualControllerWindow::buttonPressed,
            this, &LocalInputSource::onButtonPressed);
    connect(m_window.get(), &VirtualControllerWindow::buttonReleased,
            this, &LocalInputSource::onButtonReleased);
    connect(m_window.get(), &VirtualControllerWindow::analogStickMoved,
            this, &LocalInputSource::onAnalogStickMoved);
    
    m_window->show();
    m_active = true;
    
    emit connectionStatusChanged(true);
    return true;
}

void LocalInputSource::stop() {
    if (!m_active) return;
    
    qDebug() << "Stopping LocalInputSource for controller" << m_controllerId;
    
    if (m_window) {
        m_window->close();
        m_window.reset();
    }
    
    m_pressedButtons.clear();
    m_state.reset();
    m_active = false;
    
    emit connectionStatusChanged(false);
}

bool LocalInputSource::isActive() const {
    return m_active;
}

ControllerState LocalInputSource::getState() const {
    return m_state;
}

void LocalInputSource::setAnalogStick(short x, short y) {
    m_state.leftStickX = x;
    m_state.leftStickY = y;
    emit stateChanged(m_state);
}

void LocalInputSource::onButtonPressed(const QString& buttonName) {
    qDebug() << "Controller" << m_controllerId << "button pressed:" << buttonName;
    
    // Add to pressed buttons set
    m_pressedButtons.insert(buttonName);
    
    // Update D-Pad state
    if (buttonName == "up") {
        m_state.dpadUp = true;
    } else if (buttonName == "down") {
        m_state.dpadDown = true;
    } else if (buttonName == "left") {
        m_state.dpadLeft = true;
    } else if (buttonName == "right") {
        m_state.dpadRight = true;
    } else if (buttonName == "up_left") {
        m_state.dpadUp = true;
        m_state.dpadLeft = true;
    } else if (buttonName == "up_right") {
        m_state.dpadUp = true;
        m_state.dpadRight = true;
    } else if (buttonName == "down_left") {
        m_state.dpadDown = true;
        m_state.dpadLeft = true;
    } else if (buttonName == "down_right") {
        m_state.dpadDown = true;
        m_state.dpadRight = true;
    } else {
        // Regular button
        unsigned short button = mapButtonNameToXUSB(buttonName);
        if (button != 0) {
            m_state.buttons |= button;
        }
    }
    
    emit stateChanged(m_state);
}

void LocalInputSource::onButtonReleased(const QString& buttonName) {
    qDebug() << "Controller" << m_controllerId << "button released:" << buttonName;
    
    // Remove from pressed buttons set
    m_pressedButtons.remove(buttonName);
    
    // Update D-Pad state based on what's still pressed
    if (buttonName == "up" || buttonName == "up_left" || buttonName == "up_right") {
        // Check if any other "up" button is still pressed
        m_state.dpadUp = m_pressedButtons.contains("up") || 
                        m_pressedButtons.contains("up_left") || 
                        m_pressedButtons.contains("up_right");
    }
    if (buttonName == "down" || buttonName == "down_left" || buttonName == "down_right") {
        m_state.dpadDown = m_pressedButtons.contains("down") || 
                          m_pressedButtons.contains("down_left") || 
                          m_pressedButtons.contains("down_right");
    }
    if (buttonName == "left" || buttonName == "up_left" || buttonName == "down_left") {
        m_state.dpadLeft = m_pressedButtons.contains("left") || 
                          m_pressedButtons.contains("up_left") || 
                          m_pressedButtons.contains("down_left");
    }
    if (buttonName == "right" || buttonName == "up_right" || buttonName == "down_right") {
        m_state.dpadRight = m_pressedButtons.contains("right") || 
                           m_pressedButtons.contains("up_right") || 
                           m_pressedButtons.contains("down_right");
    }
    
    // Regular button
    if (buttonName != "up" && buttonName != "down" && 
        buttonName != "left" && buttonName != "right" &&
        buttonName != "up_left" && buttonName != "up_right" &&
        buttonName != "down_left" && buttonName != "down_right") {
        unsigned short button = mapButtonNameToXUSB(buttonName);
        if (button != 0) {
            m_state.buttons &= ~button;
        }
    }
    
    emit stateChanged(m_state);
}

void LocalInputSource::onAnalogStickMoved(short x, short y) {
    m_state.leftStickX = x;
    m_state.leftStickY = y;
    emit stateChanged(m_state);
}

unsigned short LocalInputSource::mapButtonNameToXUSB(const QString& buttonName) {
    if (buttonName == "a") return XUSB_GAMEPAD_A;
    if (buttonName == "b") return XUSB_GAMEPAD_B;
    if (buttonName == "x") return XUSB_GAMEPAD_X;
    if (buttonName == "y") return XUSB_GAMEPAD_Y;
    if (buttonName == "lb") return XUSB_GAMEPAD_LEFT_SHOULDER;
    if (buttonName == "rb") return XUSB_GAMEPAD_RIGHT_SHOULDER;
    if (buttonName == "start") return XUSB_GAMEPAD_START;
    if (buttonName == "select") return XUSB_GAMEPAD_BACK;
    return 0; // Unknown button
}