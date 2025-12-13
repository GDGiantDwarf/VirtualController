#include "VirtualGamepadManager.h"
#include <QDebug>
#include <cstring>

VirtualGamepadManager::VirtualGamepadManager(QObject* parent)
    : QObject(parent)
    , m_client(nullptr)
    , m_controller(nullptr)
    , m_connected(false)
{
    std::memset(&m_report, 0, sizeof(m_report));
}

VirtualGamepadManager::~VirtualGamepadManager() {
    cleanup();
}

bool VirtualGamepadManager::initialize() {
    qDebug() << "Initializing Virtual Gamepad...";
    
    // Allocate ViGEm client
    m_client = vigem_alloc();
    if (!m_client) {
        QString error = "Failed to allocate ViGEm client";
        qCritical() << error;
        emit errorOccurred(error);
        return false;
    }
    
    // Connect to driver
    VIGEM_ERROR result = vigem_connect(m_client);
    if (!VIGEM_SUCCESS(result)) {
        QString error = QString("Failed to connect to ViGEmBus driver. Error code: %1").arg(result);
        qCritical() << error;
        qCritical() << "Make sure ViGEmBus driver is installed!";
        vigem_free(m_client);
        m_client = nullptr;
        emit errorOccurred(error);
        return false;
    }
    
    // Allocate Xbox 360 controller
    m_controller = vigem_target_x360_alloc();
    if (!m_controller) {
        QString error = "Failed to allocate Xbox 360 controller";
        qCritical() << error;
        vigem_disconnect(m_client);
        vigem_free(m_client);
        m_client = nullptr;
        emit errorOccurred(error);
        return false;
    }
    
    // Add controller to driver
    result = vigem_target_add(m_client, m_controller);
    if (!VIGEM_SUCCESS(result)) {
        QString error = QString("Failed to add controller to driver. Error code: %1").arg(result);
        qCritical() << error;
        vigem_target_free(m_controller);
        vigem_disconnect(m_client);
        vigem_free(m_client);
        m_client = nullptr;
        m_controller = nullptr;
        emit errorOccurred(error);
        return false;
    }
    
    m_connected = true;
    qDebug() << "Virtual Xbox 360 gamepad initialized successfully!";
    emit connectionStatusChanged(true);
    return true;
}

void VirtualGamepadManager::cleanup() {
    if (!m_connected) return;
    
    qDebug() << "Cleaning up virtual gamepad...";
    
    if (m_controller) {
        vigem_target_remove(m_client, m_controller);
        vigem_target_free(m_controller);
        m_controller = nullptr;
    }
    
    if (m_client) {
        vigem_disconnect(m_client);
        vigem_free(m_client);
        m_client = nullptr;
    }
    
    m_connected = false;
    emit connectionStatusChanged(false);
    qDebug() << "Virtual gamepad cleaned up";
}

void VirtualGamepadManager::pressButton(XUSB_BUTTON button) {
    if (!m_connected) {
        qWarning() << "Cannot press button: controller not connected";
        return;
    }
    m_report.wButtons |= button;
    updateController();
}

void VirtualGamepadManager::releaseButton(XUSB_BUTTON button) {
    if (!m_connected) return;
    m_report.wButtons &= ~button;
    updateController();
}

void VirtualGamepadManager::releaseAllButtons() {
    if (!m_connected) return;
    m_report.wButtons = 0;
    updateController();
}

void VirtualGamepadManager::setDPad(bool up, bool down, bool left, bool right) {
    if (!m_connected) return;
    
    // Clear D-Pad bits
    m_report.wButtons &= ~(XUSB_GAMEPAD_DPAD_UP | XUSB_GAMEPAD_DPAD_DOWN | 
                           XUSB_GAMEPAD_DPAD_LEFT | XUSB_GAMEPAD_DPAD_RIGHT);
    
    // Set D-Pad bits
    if (up) m_report.wButtons |= XUSB_GAMEPAD_DPAD_UP;
    if (down) m_report.wButtons |= XUSB_GAMEPAD_DPAD_DOWN;
    if (left) m_report.wButtons |= XUSB_GAMEPAD_DPAD_LEFT;
    if (right) m_report.wButtons |= XUSB_GAMEPAD_DPAD_RIGHT;
    
    updateController();
}

void VirtualGamepadManager::setLeftStick(short x, short y) {
    if (!m_connected) return;
    m_report.sThumbLX = x;
    m_report.sThumbLY = y;
    updateController();
}

void VirtualGamepadManager::setRightStick(short x, short y) {
    if (!m_connected) return;
    m_report.sThumbRX = x;
    m_report.sThumbRY = y;
    updateController();
}

void VirtualGamepadManager::setTriggers(unsigned char left, unsigned char right) {
    if (!m_connected) return;
    m_report.bLeftTrigger = left;
    m_report.bRightTrigger = right;
    updateController();
}

void VirtualGamepadManager::updateController() {
    if (!m_connected) return;
    
    VIGEM_ERROR result = vigem_target_x360_update(m_client, m_controller, m_report);
    if (!VIGEM_SUCCESS(result)) {
        qWarning() << "Failed to update controller state. Error code:" << result;
    }
}