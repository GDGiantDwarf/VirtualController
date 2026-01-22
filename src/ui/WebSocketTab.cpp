#include "WebSocketTab.h"
#include <QDebug>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPixmap>

WebSocketTab::WebSocketTab(QWidget* parent)
    : QWidget(parent)
    , m_managerInitialized(false)
    , m_activeControllerCount(0)
    , m_basePort(8765)
{
    setupUI();
}

WebSocketTab::~WebSocketTab() {
    // Clean up all controllers
    m_controllers.clear();
    
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
        "Add up to 4 WebSocket controllers for smartphone connection.\n"
        "Each controller will show a QR code that you can scan with your mobile app.\n"
        "ViGEm will be initialized when you add the first controller.",
        this
    );
    instructionsLabel->setAlignment(Qt::AlignCenter);
    instructionsLabel->setWordWrap(true);
    instructionsLabel->setStyleSheet("color: #555; font-size: 14px; padding: 10px;");
    mainLayout->addWidget(instructionsLabel);
    
    // Add controller button
    addControllerButton = new QPushButton("📱 Add WebSocket Controller", this);
    addControllerButton->setMinimumHeight(50);
    addControllerButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #2196F3; "
        "   color: white; "
        "   font-size: 16px; "
        "   font-weight: bold; "
        "   border-radius: 5px; "
        "   padding: 10px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #1976D2;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #0D47A1;"
        "}"
        "QPushButton:disabled {"
        "   background-color: #cccccc;"
        "   color: #666666;"
        "}"
    );
    connect(addControllerButton, &QPushButton::clicked, 
            this, &WebSocketTab::onAddControllerClicked);
    mainLayout->addWidget(addControllerButton);
    
    // Scroll area for controllers
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    scrollContent = new QWidget();
    controllersLayout = new QVBoxLayout(scrollContent);
    controllersLayout->setSpacing(15);
    scrollArea->setWidget(scrollContent);
    
    mainLayout->addWidget(scrollArea);
    
    // Status label
    statusLabel = new QLabel("No WebSocket controllers connected", this);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("color: #888; font-style: italic;");
    mainLayout->addWidget(statusLabel);
    
    setLayout(mainLayout);
}

void WebSocketTab::onAddControllerClicked() {
    // Check if we've reached the maximum
    if (m_activeControllerCount >= MultiControllerManager::MAX_CONTROLLERS) {
        QMessageBox::warning(this, "Maximum Reached",
            QString("Maximum of %1 controllers already added!")
                   .arg(MultiControllerManager::MAX_CONTROLLERS));
        return;
    }
    
    // Initialize manager if needed
    if (!m_managerInitialized) {
        initializeManagerIfNeeded();
        if (!m_managerInitialized) {
            return;
        }
    }
    
    // Determine next controller ID
    int controllerId = m_activeControllerCount + 1;
    
    qDebug() << "Adding WebSocket controller" << controllerId;
    
    // Add controller to manager
    if (!m_manager->addController(controllerId)) {
        QMessageBox::critical(this, "Error",
            QString("Failed to add controller %1 to ViGEm").arg(controllerId));
        return;
    }
    
    // Create WebSocket input source with unique port
    quint16 port = m_basePort + m_activeControllerCount;
    auto inputSource = std::make_unique<WebSocketInputSource>(controllerId, this);
    
    // Connect signals BEFORE starting
    connect(inputSource.get(), &IInputSource::stateChanged,
            this, &WebSocketTab::onInputStateChanged);
    connect(inputSource.get(), &IInputSource::connectionStatusChanged,
            this, &WebSocketTab::onInputConnectionChanged);
    connect(inputSource.get(), &IInputSource::errorOccurred,
            this, &WebSocketTab::onInputError);
    
    // Start WebSocket server
    if (!inputSource->start()) {
        m_manager->removeController(controllerId);
        QMessageBox::critical(this, "Error",
            QString("Failed to start WebSocket server for controller %1").arg(controllerId));
        return;
    }
    
    // Create controller widget with QR code
    QWidget* controllerWidget = createControllerWidget(controllerId, inputSource.get());
    controllersLayout->addWidget(controllerWidget);
    
    // Store controller info
    WebSocketControllerInfo info(controllerId);
    info.inputSource = std::move(inputSource);
    
    // Find the labels and button we just created
    QList<QLabel*> labels = controllerWidget->findChildren<QLabel*>();
    for (QLabel* label : labels) {
        if (label->objectName() == "statusLabel") {
            info.statusLabel = label;
        } else if (label->objectName() == "qrCodeLabel") {
            info.qrCodeLabel = label;
        }
    }
    info.removeButton = controllerWidget->findChild<QPushButton*>("removeButton");
    
    m_controllers.push_back(std::move(info));
    
    m_activeControllerCount++;
    updateControllerStatus();
    
    // Disable add button if max reached
    if (m_activeControllerCount >= MultiControllerManager::MAX_CONTROLLERS) {
        addControllerButton->setEnabled(false);
        addControllerButton->setText("Maximum Controllers Reached");
    }
}

void WebSocketTab::onRemoveControllerClicked(int controllerId) {
    qDebug() << "Removing WebSocket controller" << controllerId;
    
    // Remove from manager
    if (m_manager) {
        m_manager->removeController(controllerId);
    }
    
    // Find and remove controller
    auto it = std::find_if(m_controllers.begin(), m_controllers.end(),
        [controllerId](const WebSocketControllerInfo& info) {
            return info.controllerId == controllerId;
        });
    
    if (it != m_controllers.end()) {
        // Stop the WebSocket server
        if (it->inputSource) {
            it->inputSource->stop();
        }
        
        // Remove widget
        if (it->removeButton) {
            QWidget* parentWidget = it->removeButton->parentWidget();
            if (parentWidget) {
                parentWidget->deleteLater();
            }
        }
        
        // Remove from vector
        m_controllers.erase(it);
    }
    
    m_activeControllerCount--;
    updateControllerStatus();
    
    // Re-enable add button
    addControllerButton->setEnabled(true);
    addControllerButton->setText("📱 Add WebSocket Controller");
}

void WebSocketTab::onInputStateChanged(const ControllerState& state) {
    if (m_manager && m_manager->isClientConnected()) {
        m_manager->updateController(state.controllerId, state);
    }
}

void WebSocketTab::onInputConnectionChanged(bool connected) {
    // Update status label for this specific controller
    WebSocketInputSource* source = qobject_cast<WebSocketInputSource*>(sender());
    if (!source) return;
    
    auto it = std::find_if(m_controllers.begin(), m_controllers.end(),
        [source](const WebSocketControllerInfo& info) {
            return info.inputSource.get() == source;
        });
    
    if (it != m_controllers.end() && it->statusLabel) {
        if (connected) {
            it->statusLabel->setText(QString("Status: ✓ Mobile Connected (%1 clients)")
                                    .arg(source->getConnectedClients()));
            it->statusLabel->setStyleSheet(
                "padding: 8px; "
                "background-color: #ccffcc; "
                "border-radius: 5px; "
                "font-weight: bold; "
                "color: #006600;"
            );
        } else {
            it->statusLabel->setText("Status: ⏳ Waiting for connection...");
            it->statusLabel->setStyleSheet(
                "padding: 8px; "
                "background-color: #fff3cd; "
                "border-radius: 5px; "
                "font-weight: bold; "
                "color: #856404;"
            );
        }
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

void WebSocketTab::updateControllerStatus() {
    if (m_activeControllerCount == 0) {
        statusLabel->setText("No WebSocket controllers connected");
        statusLabel->setStyleSheet("color: #888; font-style: italic;");
    } else {
        statusLabel->setText(QString("%1 WebSocket controller(s) active")
                                    .arg(m_activeControllerCount));
        statusLabel->setStyleSheet("color: green; font-weight: bold;");
    }
}

QWidget* WebSocketTab::createControllerWidget(int controllerId, WebSocketInputSource* source) {
    QWidget* widget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(widget);
    
    // Group box for this controller
    QGroupBox* groupBox = new QGroupBox(QString("Controller %1").arg(controllerId));
    groupBox->setStyleSheet(
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
    
    QVBoxLayout* groupLayout = new QVBoxLayout(groupBox);
    
    // Connection info
    QString connectionUrl = source->getConnectionUrl();
    QLabel* urlLabel = new QLabel(QString("Connection URL: <b>%1</b>").arg(connectionUrl), this);
    urlLabel->setWordWrap(true);
    urlLabel->setStyleSheet("padding: 5px; background-color: #f0f0f0; border-radius: 3px;");
    groupLayout->addWidget(urlLabel);
    
    // QR Code
    QLabel* qrLabel = new QLabel(this);
    qrLabel->setObjectName("qrCodeLabel");
    qrLabel->setAlignment(Qt::AlignCenter);
    QImage qrImage = generateQrCode(connectionUrl);
    qrLabel->setPixmap(QPixmap::fromImage(qrImage));
    groupLayout->addWidget(qrLabel);
    
    QLabel* qrInstructionLabel = new QLabel("Scan this QR code with your mobile app to connect", this);
    qrInstructionLabel->setAlignment(Qt::AlignCenter);
    qrInstructionLabel->setStyleSheet("color: #666; font-style: italic; font-size: 12px;");
    groupLayout->addWidget(qrInstructionLabel);
    
    // Status label
    QLabel* statusLbl = new QLabel("Status: ⏳ Waiting for connection...", this);
    statusLbl->setObjectName("statusLabel");
    statusLbl->setAlignment(Qt::AlignCenter);
    statusLbl->setStyleSheet(
        "padding: 8px; "
        "background-color: #fff3cd; "
        "border-radius: 5px; "
        "font-weight: bold; "
        "color: #856404;"
    );
    groupLayout->addWidget(statusLbl);
    
    // Remove button
    QPushButton* removeBtn = new QPushButton(QString("❌ Remove Controller %1").arg(controllerId), this);
    removeBtn->setObjectName("removeButton");
    removeBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: #f44336; "
        "   color: white; "
        "   padding: 10px; "
        "   border-radius: 5px; "
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #da190b;"
        "}"
    );
    connect(removeBtn, &QPushButton::clicked, this, [this, controllerId]() {
        onRemoveControllerClicked(controllerId);
    });
    groupLayout->addWidget(removeBtn);
    
    layout->addWidget(groupBox);
    widget->setLayout(layout);
    
    return widget;
}

QImage WebSocketTab::generateQrCode(const QString& url) {
    // Generate QR code with appropriate size
    QImage qrImage = m_qrGenerator.generateQr(url, 300, 1);
    return qrImage;
}
