#include "MultiControllerManager.h"
#include <QDebug>
#include <QThread>
#include <cstring>

MultiControllerManager::MultiControllerManager(QObject* parent)
    : QObject(parent)
    , m_client(nullptr)
    , m_clientConnected(false)
    , m_retryCount(0)
{
}

MultiControllerManager::~MultiControllerManager() {
    cleanup();
}

bool MultiControllerManager::initialize() {
    qDebug() << "Initializing MultiControllerManager...";
    return initializeClientWithRetry();
}

bool MultiControllerManager::initializeClientWithRetry() {
    m_retryCount = 0;
    
    while (m_retryCount < MAX_RETRY_ATTEMPTS) {
        qDebug() << "Attempting to connect to ViGEmBus (attempt" 
                 << (m_retryCount + 1) << "of" << MAX_RETRY_ATTEMPTS << ")";
        
        if (connectToViGEm()) {
            m_clientConnected = true;
            emit clientConnectionChanged(true);
            qDebug() << "Successfully connected to ViGEmBus!";
            return true;
        }
        
        m_retryCount++;
        
        if (m_retryCount < MAX_RETRY_ATTEMPTS) {
            qWarning() << "Connection failed. Retrying in" << RETRY_DELAY_MS << "ms...";
            QThread::msleep(RETRY_DELAY_MS);
        }
    }
    
    QString error = QString("Failed to connect to ViGEmBus after %1 attempts. "
                           "Please ensure ViGEmBus driver is installed.")
                           .arg(MAX_RETRY_ATTEMPTS);
    qCritical() << error;
    emit errorOccurred(error);
    return false;
}

bool MultiControllerManager::connectToViGEm() {
    // Allocate ViGEm client
    m_client = vigem_alloc();
    if (!m_client) {
        qCritical() << "Failed to allocate ViGEm client";
        return false;
    }
    
    // Connect to driver
    VIGEM_ERROR result = vigem_connect(m_client);
    if (!VIGEM_SUCCESS(result)) {
        qWarning() << "Failed to connect to ViGEmBus driver. Error code:" << result;
        vigem_free(m_client);
        m_client = nullptr;
        return false;
    }
    
    return true;
}

bool MultiControllerManager::addController(int controllerId) {
    if (controllerId < 1 || controllerId > MAX_CONTROLLERS) {
        QString error = QString("Invalid controller ID: %1. Must be between 1 and %2")
                               .arg(controllerId).arg(MAX_CONTROLLERS);
        qWarning() << error;
        emit errorOccurred(error);
        return false;
    }
    
    if (!m_clientConnected) {
        qWarning() << "Cannot add controller: ViGEm client not connected";
        return false;
    }
    
    // Check if controller already exists
    if (getController(controllerId) != nullptr) {
        qWarning() << "Controller" << controllerId << "already exists";
        return true; // Not an error, just already exists
    }
    
    qDebug() << "Adding virtual controller" << controllerId;
    
    // Create new controller
    auto controller = std::make_unique<VirtualController>(controllerId);
    
    // Allocate Xbox 360 controller
    controller->target = vigem_target_x360_alloc();
    if (!controller->target) {
        QString error = QString("Failed to allocate Xbox 360 controller %1").arg(controllerId);
        qCritical() << error;
        emit errorOccurred(error);
        return false;
    }
    
    // Add controller to driver
    VIGEM_ERROR result = vigem_target_add(m_client, controller->target);
    if (!VIGEM_SUCCESS(result)) {
        QString error = QString("Failed to add controller %1 to driver. Error code: %2")
                               .arg(controllerId).arg(result);
        qCritical() << error;
        vigem_target_free(controller->target);
        emit errorOccurred(error);
        return false;
    }
    
    controller->connected = true;
    m_controllers.emplace_back(std::move(controller));
    
    qDebug() << "Virtual controller" << controllerId << "added successfully!";
    emit controllerAdded(controllerId);
    return true;
}

void MultiControllerManager::removeController(int controllerId) {
    VirtualController* controller = getController(controllerId);
    if (!controller) {
        qWarning() << "Controller" << controllerId << "not found";
        return;
    }
    
    qDebug() << "Removing virtual controller" << controllerId;
    
    if (controller->target) {
        vigem_target_remove(m_client, controller->target);
        vigem_target_free(controller->target);
    }
    
    // Remove from vector
    m_controllers.erase(
        std::remove_if(m_controllers.begin(), m_controllers.end(),
            [controllerId](const std::unique_ptr<VirtualController>& ctrl) {
                return ctrl->controllerId == controllerId;
            }),
        m_controllers.end()
    );
    
    emit controllerRemoved(controllerId);
    qDebug() << "Controller" << controllerId << "removed";
}

void MultiControllerManager::updateController(int controllerId, const ControllerState& state) {
    VirtualController* controller = getController(controllerId);
    if (!controller || !controller->connected) {
        return;
    }
    
    controller->report = convertStateToReport(state);
    
    VIGEM_ERROR result = vigem_target_x360_update(m_client, controller->target, controller->report);
    if (!VIGEM_SUCCESS(result)) {
        qWarning() << "Failed to update controller" << controllerId 
                   << "state. Error code:" << result;
    }
}

void MultiControllerManager::cleanup() {
    if (!m_clientConnected) return;
    
    qDebug() << "Cleaning up MultiControllerManager...";
    
    // Remove all controllers
    while (!m_controllers.empty()) {
        removeController(m_controllers[0]->controllerId);
    }
    
    // Disconnect client
    if (m_client) {
        vigem_disconnect(m_client);
        vigem_free(m_client);
        m_client = nullptr;
    }
    
    m_clientConnected = false;
    emit clientConnectionChanged(false);
    qDebug() << "MultiControllerManager cleaned up";
}

bool MultiControllerManager::isControllerConnected(int controllerId) const {
    for (const auto& controller : m_controllers) {
        if (controller->controllerId == controllerId) {
            return controller->connected;
        }
    }
    return false;
}

int MultiControllerManager::getActiveControllerCount() const {
    return static_cast<int>(m_controllers.size());
}

XUSB_REPORT MultiControllerManager::convertStateToReport(const ControllerState& state) {
    XUSB_REPORT report;
    std::memset(&report, 0, sizeof(report));
    
    // Buttons
    report.wButtons = state.buttons;
    
    // D-Pad
    if (state.dpadUp) report.wButtons |= XUSB_GAMEPAD_DPAD_UP;
    if (state.dpadDown) report.wButtons |= XUSB_GAMEPAD_DPAD_DOWN;
    if (state.dpadLeft) report.wButtons |= XUSB_GAMEPAD_DPAD_LEFT;
    if (state.dpadRight) report.wButtons |= XUSB_GAMEPAD_DPAD_RIGHT;
    
    // Analog sticks
    report.sThumbLX = state.leftStickX;
    report.sThumbLY = state.leftStickY;
    report.sThumbRX = state.rightStickX;
    report.sThumbRY = state.rightStickY;
    
    // Triggers
    report.bLeftTrigger = state.leftTrigger;
    report.bRightTrigger = state.rightTrigger;
    
    return report;
}

VirtualController* MultiControllerManager::getController(int controllerId) {
    for (auto& controller : m_controllers) {
        if (controller->controllerId == controllerId) {
            return controller.get();
        }
    }
    return nullptr;
}