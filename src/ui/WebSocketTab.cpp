#include "WebSocketTab.h"
#include <QDebug>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPixmap>
#include <algorithm>

WebSocketTab::WebSocketTab(QWidget* parent)
    : QWidget(parent)
    , m_managerInitialized(false)
{
    setupUI();
    initializeAllServers();
}

WebSocketTab::~WebSocketTab() {
    // Clean up all input sources
    for (auto& slot : m_controllerSlots) {
        if (slot.inputSource && slot.inputSource->isActive()) {
            slot.inputSource->stop();
        }
    }
    
    // Clean up manager
    if (m_manager) {
        m_manager->cleanup();
    }
}

void WebSocketTab::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    
    // Title
    QLabel* titleLabel = new QLabel("WebSocket Controller Management", this);
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold;");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);
    
    // Manager status label
    managerStatusLabel = new QLabel("ViGEm Manager: Not initialized", this);
    managerStatusLabel->setAlignment(Qt::AlignCenter);
    managerStatusLabel->setStyleSheet(
        "padding: 10px; "
        "background-color: #e0e0e0; "
        "border-radius: 5px; "
        "font-weight: bold;"
    );
    mainLayout->addWidget(managerStatusLabel);
    
    // Instructions
    QLabel* instructionsLabel = new QLabel(
        "4 WebSocket servers are always listening on ports 8765-8768.\n"
        "Scan the QR code below to connect with your mobile app.\n"
        "A virtual controller will be created when a device connects.",
        this
    );
    instructionsLabel->setAlignment(Qt::AlignCenter);
    instructionsLabel->setWordWrap(true);
    instructionsLabel->setStyleSheet("color: #555; font-size: 14px; padding: 10px;");
    mainLayout->addWidget(instructionsLabel);
    
    // QR Code Section
    QGroupBox* qrGroupBox = new QGroupBox("Connect Mobile Controller", this);
    qrGroupBox->setStyleSheet(
        "QGroupBox {"
        "   font-weight: bold; "
        "   border: 2px solid #2196F3; "
        "   border-radius: 8px; "
        "   margin-top: 10px; "
        "   padding-top: 10px;"
        "}"
        "QGroupBox::title {"
        "   subcontrol-origin: margin; "
        "   left: 10px; "
        "   padding: 0 5px;"
        "}"
    );
    QVBoxLayout* qrLayout = new QVBoxLayout(qrGroupBox);
    
    // Connection URL
    urlLabel = new QLabel("Connection URL: Not available", this);
    urlLabel->setAlignment(Qt::AlignCenter);
    urlLabel->setWordWrap(true);
    urlLabel->setStyleSheet("padding: 5px; background-color: #f0f0f0; border-radius: 3px; font-weight: bold;");
    qrLayout->addWidget(urlLabel);
    
    // QR Code
    qrCodeLabel = new QLabel(this);
    qrCodeLabel->setAlignment(Qt::AlignCenter);
    qrCodeLabel->setMinimumSize(300, 300);
    qrLayout->addWidget(qrCodeLabel);
    
    QLabel* qrInstructionLabel = new QLabel("Scan this QR code with your mobile app to connect", this);
    qrInstructionLabel->setAlignment(Qt::AlignCenter);
    qrInstructionLabel->setStyleSheet("color: #666; font-style: italic; font-size: 12px;");
    qrLayout->addWidget(qrInstructionLabel);
    
    mainLayout->addWidget(qrGroupBox);
    
    // Status label
    statusLabel = new QLabel("Initializing WebSocket servers...", this);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("color: #888; font-style: italic;");
    mainLayout->addWidget(statusLabel);
    
    mainLayout->addStretch();
    setLayout(mainLayout);
}

void WebSocketTab::initializeAllServers() {
    qDebug() << "Initializing all 4 WebSocket servers...";
    
    // Create 4 input sources on ports 8765-8768
    for (int i = 0; i < MAX_SERVERS; ++i) {
        ControllerSlot& slot = m_controllerSlots[i];
        slot.port = m_basePort + i;
        slot.vigEmControllerId = -1;  // Not created yet
        slot.isConnected = false;
        
        auto inputSource = std::make_unique<WebSocketInputSource>(i + 1, this);
        inputSource->setPort(slot.port);
        
        // Connect signals BEFORE starting
        connect(inputSource.get(), &IInputSource::stateChanged,
                this, &WebSocketTab::onInputStateChanged);
        connect(inputSource.get(), &IInputSource::connectionStatusChanged,
                this, &WebSocketTab::onInputConnectionChanged);
        connect(inputSource.get(), &IInputSource::errorOccurred,
                this, &WebSocketTab::onInputError);
        
        // Start WebSocket server
        if (!inputSource->start()) {
            qWarning() << "Failed to start WebSocket server on port" << slot.port;
            statusLabel->setText(QString("❌ Failed to start server on port %1").arg(slot.port));
            continue;
        }
        
        qInfo() << "Server" << i + 1 << "started on port" << slot.port;
        slot.inputSource = std::move(inputSource);
    }
    
    // Update QR code for the first (available) server
    updateQrCode();
}

void WebSocketTab::updateQrCode() {
    int lowestIndex = getLowestAvailableSlotIndex();
    
    if (lowestIndex >= 0 && lowestIndex < MAX_SERVERS) {
        const ControllerSlot& slot = m_controllerSlots[lowestIndex];
        QString connectionUrl = slot.inputSource->getConnectionUrl();
        
        // Update URL label
        urlLabel->setText(QString("Connection URL: <b>%1</b>").arg(connectionUrl));
        
        // Generate and display QR code
        QImage qrImage = generateQrCode(connectionUrl);
        qrCodeLabel->setPixmap(QPixmap::fromImage(qrImage));
        
        // Count connected slots
        int connectedCount = 0;
        for (const auto& s : m_controllerSlots) {
            if (s.isConnected) connectedCount++;
        }
        
        // Update status
        statusLabel->setText(QString("%1/4 connected | Next available: Port %2")
                                    .arg(connectedCount)
                                    .arg(slot.port));
    } else {
        statusLabel->setText("❌ All 4 controllers connected");
    }
}

int WebSocketTab::getLowestAvailableSlotIndex() const {
    for (int i = 0; i < MAX_SERVERS; ++i) {
        if (m_controllerSlots[i].inputSource && !m_controllerSlots[i].isConnected) {
            return i;
        }
    }
    return -1; // All slots have clients
}

int WebSocketTab::getSlotIndexForInputSource(WebSocketInputSource* source) const {
    for (int i = 0; i < MAX_SERVERS; ++i) {
        if (m_controllerSlots[i].inputSource.get() == source) {
            return i;
        }
    }
    return -1;
}

void WebSocketTab::onInputConnectionChanged(bool connected) {
    WebSocketInputSource* source = qobject_cast<WebSocketInputSource*>(sender());
    if (!source) return;
    
    // Find which slot this is
    int slotIndex = getSlotIndexForInputSource(source);
    if (slotIndex < 0) return;
    
    ControllerSlot& slot = m_controllerSlots[slotIndex];
    
    if (connected) {
        qDebug() << "Client connected to port" << slot.port;
        
        slot.isConnected = true;
        
        // Create ViGEm controller only when client connects
        createVigEmControllerForSlot(slotIndex);
        
        // Update QR code to show next available port
        updateQrCode();
    } else {
        qDebug() << "Client disconnected from port" << slot.port;
        
        slot.isConnected = false;
        
        // Remove ViGEm controller when client disconnects
        removeVigEmControllerForSlot(slotIndex);
        
        // Update QR code
        updateQrCode();
    }
}

void WebSocketTab::createVigEmControllerForSlot(int slotIndex) {
    if (slotIndex < 0 || slotIndex >= MAX_SERVERS) return;
    
    ControllerSlot& slot = m_controllerSlots[slotIndex];
    
    // Skip if already has a ViGEm controller
    if (slot.vigEmControllerId >= 0) return;
    
    // Initialize manager if needed
    if (!m_managerInitialized) {
        initializeManagerIfNeeded();
        if (!m_managerInitialized) {
            return;
        }
    }
    
    int controllerId = slotIndex + 1;
    
    qDebug() << "Creating ViGEm controller" << controllerId << "for port" << slot.port;
    
    // Add controller to manager
    if (!m_manager->addController(controllerId)) {
        qWarning() << "Failed to add controller" << controllerId << "to ViGEm";
        return;
    }
    
    slot.vigEmControllerId = controllerId;
}

void WebSocketTab::removeVigEmControllerForSlot(int slotIndex) {
    if (slotIndex < 0 || slotIndex >= MAX_SERVERS) return;
    
    ControllerSlot& slot = m_controllerSlots[slotIndex];
    
    // Skip if no ViGEm controller
    if (slot.vigEmControllerId < 0) return;
    
    int controllerId = slot.vigEmControllerId;
    
    qDebug() << "Removing ViGEm controller" << controllerId;
    
    if (m_manager) {
        m_manager->removeController(controllerId);
    }
    
    slot.vigEmControllerId = -1;
}

void WebSocketTab::onInputStateChanged(const ControllerState& state) {
    if (m_manager && m_manager->isClientConnected()) {
        m_manager->updateController(state.controllerId, state);
    }
}

void WebSocketTab::onInputError(const QString& error) {
    QMessageBox::warning(this, "WebSocket Error", error);
}

void WebSocketTab::onManagerConnectionChanged(bool connected) {
    if (connected) {
        managerStatusLabel->setText("ViGEm Manager: ✓ Connected");
        managerStatusLabel->setStyleSheet(
            "padding: 10px; "
            "background-color: #ccffcc; "
            "border-radius: 5px; "
            "font-weight: bold; "
            "color: #006600;"
        );
    } else {
        managerStatusLabel->setText("ViGEm Manager: ✗ Disconnected");
        managerStatusLabel->setStyleSheet(
            "padding: 10px; "
            "background-color: #ffcccc; "
            "border-radius: 5px; "
            "font-weight: bold; "
            "color: #660000;"
        );
    }
}

void WebSocketTab::onManagerError(const QString& error) {
    qCritical() << "Manager error:" << error;
    managerStatusLabel->setText(QString("ViGEm Error: %1").arg(error));
    managerStatusLabel->setStyleSheet(
        "padding: 10px; "
        "background-color: #ffcccc; "
        "border-radius: 5px; "
        "font-weight: bold; "
        "color: #cc0000;"
    );
}

void WebSocketTab::onControllerAdded(int controllerId) {
    qDebug() << "Controller" << controllerId << "successfully added to manager";
}

void WebSocketTab::onControllerRemoved(int controllerId) {
    qDebug() << "Controller" << controllerId << "removed from manager";
}

void WebSocketTab::initializeManagerIfNeeded() {
    if (m_managerInitialized) return;
    
    qDebug() << "Initializing MultiControllerManager for WebSocket...";
    
    // Create manager
    m_manager = std::make_unique<MultiControllerManager>(this);
    
    // Connect signals
    connect(m_manager.get(), &MultiControllerManager::clientConnectionChanged,
            this, &WebSocketTab::onManagerConnectionChanged);
    connect(m_manager.get(), &MultiControllerManager::errorOccurred,
            this, &WebSocketTab::onManagerError);
    connect(m_manager.get(), &MultiControllerManager::controllerAdded,
            this, &WebSocketTab::onControllerAdded);
    connect(m_manager.get(), &MultiControllerManager::controllerRemoved,
            this, &WebSocketTab::onControllerRemoved);
    
    // Initialize with retry
    if (!m_manager->initialize()) {
        QMessageBox::critical(this, "ViGEm Error",
            "Failed to initialize virtual gamepad manager!\n\n"
            "Make sure ViGEmBus driver is installed.\n"
            "Download from: https://github.com/nefarius/ViGEmBus/releases\n\n"
            "The application tried to connect 3 times but failed.");
        m_manager.reset();
        return;
    }
    
    m_managerInitialized = true;
    qDebug() << "MultiControllerManager initialized successfully!";
}

QImage WebSocketTab::generateQrCode(const QString& url) {
    QImage qrImage = m_qrGenerator.generateQr(url, 300, 1);
    return qrImage;
}
