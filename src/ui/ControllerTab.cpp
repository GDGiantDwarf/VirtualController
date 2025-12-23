#include "ControllerTab.h"
#include <QDebug>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QGroupBox>

ControllerTab::ControllerTab(QWidget* parent)
    : QWidget(parent)
    , m_managerInitialized(false)
    , m_activeControllerCount(0)
{
    setupUI();
}

ControllerTab::~ControllerTab() {
    // Clean up all input sources
    m_inputSources.clear();
    
    // Clean up manager
    if (m_manager) {
        m_manager->cleanup();
    }
}

void ControllerTab::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(20);
    
    // Title
    QLabel* titleLabel = new QLabel("Local Controller Management", this);
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold;");
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);
    
    // Manager status label
    managerStatusLabel = new QLabel("ViGEm Manager: Not initialized", this);
    managerStatusLabel->setAlignment(Qt::AlignCenter);
    managerStatusLabel->setStyleSheet(
        "padding: 10px; "
        "background-color: #e0e0e0; "
        "border-radius: 5px; "
        "font-weight: bold;"
    );
    layout->addWidget(managerStatusLabel);
    
    // Instructions
    QLabel* instructionsLabel = new QLabel(
        "Add up to 4 local controllers for testing.\n"
        "Each controller opens a separate window with buttons and analog stick.\n"
        "ViGEm will be initialized when you add the first controller.",
        this
    );
    instructionsLabel->setAlignment(Qt::AlignCenter);
    instructionsLabel->setWordWrap(true);
    instructionsLabel->setStyleSheet("color: #555; font-size: 14px; padding: 10px;");
    layout->addWidget(instructionsLabel);
    
    // Add controller button
    addControllerButton = new QPushButton("➕ Add Controller", this);
    addControllerButton->setMinimumHeight(50);
    addControllerButton->setStyleSheet(
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
        "QPushButton:disabled {"
        "   background-color: #cccccc;"
        "   color: #666666;"
        "}"
    );
    connect(addControllerButton, &QPushButton::clicked, 
            this, &ControllerTab::onAddControllerClicked);
    layout->addWidget(addControllerButton);
    
    // Controller buttons area
    QGroupBox* controllersGroup = new QGroupBox("Active Controllers", this);
    controllerButtonsLayout = new QVBoxLayout();
    controllerButtonsWidget = new QWidget(this);
    controllerButtonsWidget->setLayout(controllerButtonsLayout);
    
    QVBoxLayout* groupLayout = new QVBoxLayout(controllersGroup);
    groupLayout->addWidget(controllerButtonsWidget);
    layout->addWidget(controllersGroup);
    
    // Status label
    statusLabel = new QLabel("No controllers connected", this);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("color: #888; font-style: italic;");
    layout->addWidget(statusLabel);
    
    layout->addStretch();
    
    setLayout(layout);
}

void ControllerTab::onAddControllerClicked() {
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
            // Initialization failed
            return;
        }
    }
    
    // Determine next controller ID
    int controllerId = m_activeControllerCount + 1;
    
    qDebug() << "Adding controller" << controllerId;
    
    // Add controller to manager
    if (!m_manager->addController(controllerId)) {
        QMessageBox::critical(this, "Error",
            QString("Failed to add controller %1").arg(controllerId));
        return;
    }
    
    // Create input source
    auto inputSource = std::make_unique<LocalInputSource>(controllerId, this);
    
    // Connect signals
    connect(inputSource.get(), &IInputSource::stateChanged,
            this, &ControllerTab::onInputStateChanged);
    connect(inputSource.get(), &IInputSource::connectionStatusChanged,
            this, &ControllerTab::onInputConnectionChanged);
    connect(inputSource.get(), &IInputSource::errorOccurred,
            this, [this](const QString& error) {
                QMessageBox::warning(this, "Input Error", error);
            });
    
    // Start input source
    if (!inputSource->start()) {
        m_manager->removeController(controllerId);
        QMessageBox::critical(this, "Error",
            QString("Failed to start input source for controller %1").arg(controllerId));
        return;
    }
    
    // Add remove button
    QPushButton* removeBtn = new QPushButton(
        QString("❌ Remove Controller %1").arg(controllerId), this);
    removeBtn->setProperty("controllerId", controllerId);
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
    
    controllerButtonsLayout->addWidget(removeBtn);
    removeButtons.append(removeBtn);
    
    // Store input source
    m_inputSources.emplace_back(std::move(inputSource));
    
    m_activeControllerCount++;
    updateControllerStatus();
    
    // Disable add button if max reached
    if (m_activeControllerCount >= MultiControllerManager::MAX_CONTROLLERS) {
        addControllerButton->setEnabled(false);
        addControllerButton->setText("Maximum Controllers Reached");
    }
}

void ControllerTab::onRemoveControllerClicked(int controllerId) {
    qDebug() << "Removing controller" << controllerId;
    
    // Remove from manager
    if (m_manager) {
        m_manager->removeController(controllerId);
    }
    
    // Remove input source
    m_inputSources.erase(
        std::remove_if(m_inputSources.begin(), m_inputSources.end(),
            [controllerId](const std::unique_ptr<IInputSource>& source) {
                auto* localSource = dynamic_cast<LocalInputSource*>(source.get());
                return localSource && localSource->getState().controllerId == controllerId;
            }),
        m_inputSources.end()
    );
    
    // Remove button
    for (int i = 0; i < removeButtons.size(); i++) {
        if (removeButtons[i]->property("controllerId").toInt() == controllerId) {
            removeButtons[i]->deleteLater();
            removeButtons.removeAt(i);
            break;
        }
    }
    
    m_activeControllerCount--;
    updateControllerStatus();
    
    // Re-enable add button
    addControllerButton->setEnabled(true);
    addControllerButton->setText("➕ Add Controller");
}

void ControllerTab::onInputStateChanged(const ControllerState& state) {
    if (m_manager && m_manager->isClientConnected()) {
        m_manager->updateController(state.controllerId, state);
    }
}

void ControllerTab::onInputConnectionChanged(bool connected) {
    qDebug() << "Input connection changed:" << connected;
}

void ControllerTab::onManagerConnectionChanged(bool connected) {
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

void ControllerTab::onManagerError(const QString& error) {
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

void ControllerTab::onControllerAdded(int controllerId) {
    qDebug() << "Controller" << controllerId << "successfully added to manager";
}

void ControllerTab::onControllerRemoved(int controllerId) {
    qDebug() << "Controller" << controllerId << "removed from manager";
}

void ControllerTab::initializeManagerIfNeeded() {
    if (m_managerInitialized) return;
    
    qDebug() << "Initializing MultiControllerManager...";
    
    // Create manager
    m_manager = std::make_unique<MultiControllerManager>(this);
    
    // Connect signals
    connect(m_manager.get(), &MultiControllerManager::clientConnectionChanged,
            this, &ControllerTab::onManagerConnectionChanged);
    connect(m_manager.get(), &MultiControllerManager::errorOccurred,
            this, &ControllerTab::onManagerError);
    connect(m_manager.get(), &MultiControllerManager::controllerAdded,
            this, &ControllerTab::onControllerAdded);
    connect(m_manager.get(), &MultiControllerManager::controllerRemoved,
            this, &ControllerTab::onControllerRemoved);
    
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

void ControllerTab::updateControllerStatus() {
    if (m_activeControllerCount == 0) {
        statusLabel->setText("No controllers connected");
        statusLabel->setStyleSheet("color: #888; font-style: italic;");
    } else {
        statusLabel->setText(QString("%1 controller(s) active")
                                    .arg(m_activeControllerCount));
        statusLabel->setStyleSheet("color: green; font-weight: bold;");
    }
}