#include "VirtualControllerWindow.h"
#include <QVBoxLayout>
#include <QDebug>

VirtualControllerWindow::VirtualControllerWindow(QWidget* parent)
    : QWidget(parent, Qt::WindowStaysOnTopHint | Qt::Tool | Qt::WindowDoesNotAcceptFocus)
{
    setupUI();
    setWindowTitle("Virtual Controller");
    resize(400, 500);
    
    // Prevent window from getting focus
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::NoFocus);
}

void VirtualControllerWindow::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Status label
    statusLabel = new QLabel("Virtual Controller Active", this);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("font-size: 16px; font-weight: bold; padding: 10px;");
    statusLabel->setFocusPolicy(Qt::NoFocus);
    mainLayout->addWidget(statusLabel);
    
    // Create grid layout for buttons
    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->setSpacing(10);
    
    // D-Pad (directional buttons)
    QPushButton* upBtn = createButton("↑", "up");
    QPushButton* downBtn = createButton("↓", "down");
    QPushButton* leftBtn = createButton("←", "left");
    QPushButton* rightBtn = createButton("→", "right");
    
    gridLayout->addWidget(upBtn, 0, 1);
    gridLayout->addWidget(leftBtn, 1, 0);
    gridLayout->addWidget(new QLabel("D-Pad", this), 1, 1);
    gridLayout->addWidget(rightBtn, 1, 2);
    gridLayout->addWidget(downBtn, 2, 1);
    
    mainLayout->addLayout(gridLayout);
    
    // Action buttons layout
    QGridLayout* actionLayout = new QGridLayout();
    actionLayout->setSpacing(10);
    
    QPushButton* aBtn = createButton("A", "a");
    QPushButton* bBtn = createButton("B", "b");
    QPushButton* xBtn = createButton("X", "x");
    QPushButton* yBtn = createButton("Y", "y");
    
    aBtn->setStyleSheet("background-color: #90EE90; font-weight: bold;");
    bBtn->setStyleSheet("background-color: #FFB6C1; font-weight: bold;");
    xBtn->setStyleSheet("background-color: #ADD8E6; font-weight: bold;");
    yBtn->setStyleSheet("background-color: #FFFFE0; font-weight: bold;");
    
    actionLayout->addWidget(yBtn, 0, 1);
    actionLayout->addWidget(xBtn, 1, 0);
    actionLayout->addWidget(new QLabel("Actions", this), 1, 1);
    actionLayout->addWidget(bBtn, 1, 2);
    actionLayout->addWidget(aBtn, 2, 1);
    
    mainLayout->addLayout(actionLayout);
    
    // Shoulder buttons
    QHBoxLayout* shoulderLayout = new QHBoxLayout();
    QPushButton* lBtn = createButton("LB", "lb");
    QPushButton* rBtn = createButton("RB", "rb");
    
    shoulderLayout->addWidget(lBtn);
    shoulderLayout->addStretch();
    shoulderLayout->addWidget(rBtn);
    
    mainLayout->addLayout(shoulderLayout);
    
    // Start/Select buttons
    QHBoxLayout* systemLayout = new QHBoxLayout();
    QPushButton* selectBtn = createButton("Select", "select");
    QPushButton* startBtn = createButton("Start", "start");
    
    systemLayout->addWidget(selectBtn);
    systemLayout->addStretch();
    systemLayout->addWidget(startBtn);
    
    mainLayout->addLayout(systemLayout);
    
    mainLayout->addStretch();
    
    setLayout(mainLayout);
}

QPushButton* VirtualControllerWindow::createButton(const QString& text, const QString& buttonId) {
    QPushButton* button = new QPushButton(text, this);
    button->setMinimumSize(80, 60);
    button->setStyleSheet("font-size: 14px; font-weight: bold;");
    
    // CRITICAL: Prevent buttons from taking focus
    button->setFocusPolicy(Qt::NoFocus);
    
    buttons[buttonId] = button;
    
    // Press event
    connect(button, &QPushButton::pressed, [this, buttonId, button]() {
        qDebug() << "Button pressed:" << buttonId;
        button->setStyleSheet(button->styleSheet() + "border: 3px solid red;");
        emit buttonPressed(buttonId);
    });
    
    // Release event
    connect(button, &QPushButton::released, [this, buttonId, button]() {
        qDebug() << "Button released:" << buttonId;
        QString style = button->styleSheet();
        style.replace("border: 3px solid red;", "");
        button->setStyleSheet(style);
        emit buttonReleased(buttonId);
    });
    
    return button;
}