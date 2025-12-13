#include "ControllerTab.h"
#include <QDebug>
#include <QMessageBox>

ControllerTab::ControllerTab(QWidget* parent)
    : QWidget(parent)
    , controllerWindow(nullptr)
    , gamepadManager(new VirtualGamepadManager(this))
{
    setupUI();
    
    // Connect gamepad manager signals
    connect(gamepadManager, &VirtualGamepadManager::connectionStatusChanged,
            this, &ControllerTab::onGamepadConnectionChanged);
    connect(gamepadManager, &VirtualGamepadManager::errorOccurred,
            this, &ControllerTab::onGamepadError);
    
    // Try to initialize the virtual gamepad
    if (!gamepadManager->initialize()) {
        QMessageBox::warning(this, "ViGEm Error",
            "Failed to initialize virtual gamepad!\n\n"
            "Make sure ViGEmBus driver is installed.\n"
            "Download from: https://github.com/nefarius/ViGEmBus/releases\n\n"
            "The controller window will still work but won't create a real gamepad.");
    }
}

ControllerTab::~ControllerTab() {
    if (controllerWindow) {
        controllerWindow->close();
        delete controllerWindow;
    }
}

void ControllerTab::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(20);
    
    // Title
    QLabel* titleLabel = new QLabel("Connect a Controller", this);
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold;");
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);
    
    // Gamepad status label
    gamepadStatusLabel = new QLabel("Virtual Gamepad: Not initialized", this);
    gamepadStatusLabel->setAlignment(Qt::AlignCenter);
    gamepadStatusLabel->setStyleSheet(
        "padding: 10px; "
        "background-color: #ffcccc; "
        "border-radius: 5px; "
        "font-weight: bold;"
    );
    layout->addWidget(gamepadStatusLabel);
    
    // QR Code placeholder (for future implementation)
    qrPlaceholder = new QLabel(this);
    qrPlaceholder->setAlignment(Qt::AlignCenter);
    qrPlaceholder->setStyleSheet(
        "border: 2px dashed #888; "
        "background-color: #f0f0f0; "
        "padding: 40px; "
        "font-size: 16px; "
        "color: #666;"
    );
    qrPlaceholder->setText("QR Code will appear here\n(Future feature)");
    qrPlaceholder->setMinimumSize(300, 300);
    layout->addWidget(qrPlaceholder, 0, Qt::AlignCenter);
    
    // Instructions
    QLabel* instructionsLabel = new QLabel(
        "In the future, scan the QR code with your phone to connect.\n"
        "For now, use the virtual controller button below for testing.",
        this
    );
    instructionsLabel->setAlignment(Qt::AlignCenter);
    instructionsLabel->setWordWrap(true);
    instructionsLabel->setStyleSheet("color: #555; font-size: 14px;");
    layout->addWidget(instructionsLabel);
    
    // Open controller button (temporary for MVP)
    openControllerButton = new QPushButton("Open Virtual Controller (Test)", this);
    openControllerButton->setMinimumHeight(50);
    openControllerButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #4CAF50; "
        "   color: white; "
        "   font-size: 16px; "
        "   font-weight: bold; "
        "   border-radius: 5px; "
        "   padding: 10px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #45a049;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #3d8b40;"
        "}"
    );
    connect(openControllerButton, &QPushButton::clicked, 
            this, &ControllerTab::onOpenControllerClicked);
    layout->addWidget(openControllerButton);
    
    // Status label
    statusLabel = new QLabel("No controller window opened", this);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("color: #888; font-style: italic;");
    layout->addWidget(statusLabel);
    
    layout->addStretch();
    
    setLayout(layout);
}

void ControllerTab::onOpenControllerClicked() {
    if (!controllerWindow) {
        controllerWindow = new VirtualControllerWindow();
        
        // Connect signals
        connect(controllerWindow, &VirtualControllerWindow::buttonPressed,
                this, &ControllerTab::onControllerButtonPressed);
        connect(controllerWindow, &VirtualControllerWindow::buttonReleased,
                this, &ControllerTab::onControllerButtonReleased);
        
        statusLabel->setText("Virtual controller window opened");
        statusLabel->setStyleSheet("color: green; font-weight: bold;");
    }
    
    controllerWindow->show();
    controllerWindow->raise();
    controllerWindow->activateWindow();
}

void ControllerTab::onControllerButtonPressed(const QString& buttonName) {
    qDebug() << "ControllerTab received button press:" << buttonName;
    statusLabel->setText(QString("Button pressed: %1").arg(buttonName.toUpper()));
    
    // Send to virtual gamepad if connected
    if (gamepadManager && gamepadManager->isConnected()) {
        // Handle D-Pad specially (can press multiple directions)
        if (buttonName == "up") {
            gamepadManager->setDPad(true, false, false, false);
        } else if (buttonName == "down") {
            gamepadManager->setDPad(false, true, false, false);
        } else if (buttonName == "left") {
            gamepadManager->setDPad(false, false, true, false);
        } else if (buttonName == "right") {
            gamepadManager->setDPad(false, false, false, true);
        } else {
            // Regular buttons
            XUSB_BUTTON xButton = mapButtonNameToXUSB(buttonName);
            if (xButton != 0) {
                gamepadManager->pressButton(xButton);
            }
        }
    }
}

void ControllerTab::onControllerButtonReleased(const QString& buttonName) {
    qDebug() << "ControllerTab received button release:" << buttonName;
    statusLabel->setText(QString("Button released: %1").arg(buttonName.toUpper()));
    
    // Send to virtual gamepad if connected
    if (gamepadManager && gamepadManager->isConnected()) {
        // Handle D-Pad specially
        if (buttonName == "up" || buttonName == "down" || 
            buttonName == "left" || buttonName == "right") {
            gamepadManager->setDPad(false, false, false, false);
        } else {
            // Regular buttons
            XUSB_BUTTON xButton = mapButtonNameToXUSB(buttonName);
            if (xButton != 0) {
                gamepadManager->releaseButton(xButton);
            }
        }
    }
}

void ControllerTab::onGamepadConnectionChanged(bool connected) {
    if (connected) {
        gamepadStatusLabel->setText("Virtual Gamepad: ✓ Connected (Xbox 360 Controller)");
        gamepadStatusLabel->setStyleSheet(
            "padding: 10px; "
            "background-color: #ccffcc; "
            "border-radius: 5px; "
            "font-weight: bold; "
            "color: #006600;"
        );
    } else {
        gamepadStatusLabel->setText("Virtual Gamepad: ✗ Disconnected");
        gamepadStatusLabel->setStyleSheet(
            "padding: 10px; "
            "background-color: #ffcccc; "
            "border-radius: 5px; "
            "font-weight: bold; "
            "color: #660000;"
        );
    }
}

void ControllerTab::onGamepadError(const QString& error) {
    qCritical() << "Gamepad error:" << error;
    gamepadStatusLabel->setText(QString("Virtual Gamepad Error: %1").arg(error));
    gamepadStatusLabel->setStyleSheet(
        "padding: 10px; "
        "background-color: #ffcccc; "
        "border-radius: 5px; "
        "font-weight: bold; "
        "color: #cc0000;"
    );
}

XUSB_BUTTON ControllerTab::mapButtonNameToXUSB(const QString& buttonName) {
    if (buttonName == "a") return XUSB_GAMEPAD_A;
    if (buttonName == "b") return XUSB_GAMEPAD_B;
    if (buttonName == "x") return XUSB_GAMEPAD_X;
    if (buttonName == "y") return XUSB_GAMEPAD_Y;
    if (buttonName == "lb") return XUSB_GAMEPAD_LEFT_SHOULDER;
    if (buttonName == "rb") return XUSB_GAMEPAD_RIGHT_SHOULDER;
    if (buttonName == "start") return XUSB_GAMEPAD_START;
    if (buttonName == "select") return XUSB_GAMEPAD_BACK;
    return static_cast<XUSB_BUTTON>(0); // Unknown button
}