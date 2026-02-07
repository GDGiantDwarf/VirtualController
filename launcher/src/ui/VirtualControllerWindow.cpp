#include "VirtualControllerWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QDebug>
#include <cmath>

// ============================================================================
// AnalogStickWidget Implementation
// ============================================================================

AnalogStickWidget::AnalogStickWidget(QWidget* parent)
    : QWidget(parent)
    , m_position(0, 0)
    , m_dragging(false)
{
    setFixedSize(STICK_RADIUS * 2 + 40, STICK_RADIUS * 2 + 40);
    setMouseTracking(true);
}

void AnalogStickWidget::reset() {
    m_position = QPointF(0, 0);
    emit positionChanged(0, 0);
    update();
}

void AnalogStickWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QPointF center(width() / 2.0, height() / 2.0);
    
    // Draw background circle
    painter.setPen(QPen(Qt::black, 2));
    painter.setBrush(QColor(200, 200, 200));
    painter.drawEllipse(center, STICK_RADIUS, STICK_RADIUS);
    
    // Draw crosshair
    painter.setPen(QPen(Qt::gray, 1, Qt::DashLine));
    painter.drawLine(center.x() - STICK_RADIUS, center.y(), 
                    center.x() + STICK_RADIUS, center.y());
    painter.drawLine(center.x(), center.y() - STICK_RADIUS, 
                    center.x(), center.y() + STICK_RADIUS);
    
    // Draw stick knob
    QPointF knobPos = center + QPointF(m_position.x() * STICK_RADIUS, 
                                       -m_position.y() * STICK_RADIUS);
    painter.setPen(QPen(Qt::black, 2));
    painter.setBrush(m_dragging ? QColor(100, 150, 255) : QColor(150, 150, 255));
    painter.drawEllipse(knobPos, KNOB_RADIUS, KNOB_RADIUS);
}

void AnalogStickWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        updatePosition(event->pos());
    }
}

void AnalogStickWidget::mouseMoveEvent(QMouseEvent* event) {
    if (m_dragging) {
        updatePosition(event->pos());
    }
}

void AnalogStickWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        reset();
    }
}

void AnalogStickWidget::updatePosition(const QPointF& pos) {
    QPointF center(width() / 2.0, height() / 2.0);
    QPointF delta = pos - center;
    
    // Normalize to -1.0 to 1.0
    double distance = std::sqrt(delta.x() * delta.x() + delta.y() * delta.y());
    if (distance > STICK_RADIUS) {
        delta *= STICK_RADIUS / distance;
        distance = STICK_RADIUS;
    }
    
    m_position.setX(delta.x() / STICK_RADIUS);
    m_position.setY(-delta.y() / STICK_RADIUS); // Invert Y for standard coordinate system
    
    // Convert to Xbox 360 range (-32768 to 32767)
    short x = static_cast<short>(m_position.x() * 32767);
    short y = static_cast<short>(m_position.y() * 32767);
    
    emit positionChanged(x, y);
    update();
}

// ============================================================================
// VirtualControllerWindow Implementation
// ============================================================================

VirtualControllerWindow::VirtualControllerWindow(int controllerId, QWidget* parent)
    : QWidget(parent)
    , m_controllerId(controllerId)
{
    setWindowTitle(QString("Virtual Controller %1 (Test)").arg(controllerId));
    setupUI();
}

void VirtualControllerWindow::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Controller ID label
    controllerIdLabel = new QLabel(QString("Controller %1").arg(m_controllerId), this);
    controllerIdLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #333;");
    controllerIdLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(controllerIdLabel);
    
    // Main controller layout
    QHBoxLayout* controllerLayout = new QHBoxLayout();
    
    // Left side: D-Pad and LB
    QVBoxLayout* leftLayout = new QVBoxLayout();
    
    QPushButton* lbButton = createButton("LB", "lb");
    lbButton->setMinimumSize(100, 40);
    leftLayout->addWidget(lbButton, 0, Qt::AlignCenter);
    
    QWidget* dpadWidget = new QWidget(this);
    QGridLayout* dpadLayout = new QGridLayout(dpadWidget);
    dpadLayout->setSpacing(2);
    createDPadWithDiagonals(dpadLayout, 0, 0);
    leftLayout->addWidget(dpadWidget, 0, Qt::AlignCenter);
    
    leftLayout->addStretch();
    controllerLayout->addLayout(leftLayout);
    
    // Center: Analog stick and Select/Start
    QVBoxLayout* centerLayout = new QVBoxLayout();
    centerLayout->addStretch();
    
    analogStick = new AnalogStickWidget(this);
    connect(analogStick, &AnalogStickWidget::positionChanged,
            this, &VirtualControllerWindow::analogStickMoved);
    centerLayout->addWidget(analogStick, 0, Qt::AlignCenter);
    
    QHBoxLayout* systemButtonsLayout = new QHBoxLayout();
    systemButtonsLayout->addWidget(createButton("SELECT", "select"));
    systemButtonsLayout->addWidget(createButton("START", "start"));
    centerLayout->addLayout(systemButtonsLayout);
    
    centerLayout->addStretch();
    controllerLayout->addLayout(centerLayout);
    
    // Right side: Action buttons and RB
    QVBoxLayout* rightLayout = new QVBoxLayout();
    
    QPushButton* rbButton = createButton("RB", "rb");
    rbButton->setMinimumSize(100, 40);
    rightLayout->addWidget(rbButton, 0, Qt::AlignCenter);
    
    QWidget* actionWidget = new QWidget(this);
    QGridLayout* actionLayout = new QGridLayout(actionWidget);
    actionLayout->setSpacing(10);
    createActionButtons(actionLayout, 0, 0);
    rightLayout->addWidget(actionWidget, 0, Qt::AlignCenter);
    
    rightLayout->addStretch();
    controllerLayout->addLayout(rightLayout);
    
    mainLayout->addLayout(controllerLayout);
    
    // Status label
    statusLabel = new QLabel("Ready", this);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("color: #666; font-style: italic; padding: 10px;");
    mainLayout->addWidget(statusLabel);
    
    setLayout(mainLayout);
    resize(600, 400);
}

void VirtualControllerWindow::createDPadWithDiagonals(QGridLayout* layout, int row, int col) {
    // Create 3x3 grid for D-Pad with diagonals
    QPushButton* upLeftBtn = createButton("↖", "up_left");
    upLeftBtn->setFixedSize(50, 50);
    layout->addWidget(upLeftBtn, row, col);
    
    QPushButton* upBtn = createButton("↑", "up");
    upBtn->setFixedSize(50, 50);
    layout->addWidget(upBtn, row, col + 1);
    
    QPushButton* upRightBtn = createButton("↗", "up_right");
    upRightBtn->setFixedSize(50, 50);
    layout->addWidget(upRightBtn, row, col + 2);
    
    QPushButton* leftBtn = createButton("←", "left");
    leftBtn->setFixedSize(50, 50);
    layout->addWidget(leftBtn, row + 1, col);
    
    // Center (empty or logo)
    QLabel* centerLabel = new QLabel("⊙", this);
    centerLabel->setAlignment(Qt::AlignCenter);
    centerLabel->setFixedSize(50, 50);
    centerLabel->setStyleSheet("font-size: 24px; color: #888;");
    layout->addWidget(centerLabel, row + 1, col + 1);
    
    QPushButton* rightBtn = createButton("→", "right");
    rightBtn->setFixedSize(50, 50);
    layout->addWidget(rightBtn, row + 1, col + 2);
    
    QPushButton* downLeftBtn = createButton("↙", "down_left");
    downLeftBtn->setFixedSize(50, 50);
    layout->addWidget(downLeftBtn, row + 2, col);
    
    QPushButton* downBtn = createButton("↓", "down");
    downBtn->setFixedSize(50, 50);
    layout->addWidget(downBtn, row + 2, col + 1);
    
    QPushButton* downRightBtn = createButton("↘", "down_right");
    downRightBtn->setFixedSize(50, 50);
    layout->addWidget(downRightBtn, row + 2, col + 2);
}

void VirtualControllerWindow::createActionButtons(QGridLayout* layout, int row, int col) {
    // Xbox layout: Y on top, X left, B right, A bottom
    QPushButton* yBtn = createButton("Y", "y");
    yBtn->setFixedSize(60, 60);
    yBtn->setStyleSheet(yBtn->styleSheet() + "background-color: #FFD700;");
    layout->addWidget(yBtn, row, col + 1);
    
    QPushButton* xBtn = createButton("X", "x");
    xBtn->setFixedSize(60, 60);
    xBtn->setStyleSheet(xBtn->styleSheet() + "background-color: #4169E1;");
    layout->addWidget(xBtn, row + 1, col);
    
    QPushButton* bBtn = createButton("B", "b");
    bBtn->setFixedSize(60, 60);
    bBtn->setStyleSheet(bBtn->styleSheet() + "background-color: #DC143C;");
    layout->addWidget(bBtn, row + 1, col + 2);
    
    QPushButton* aBtn = createButton("A", "a");
    aBtn->setFixedSize(60, 60);
    aBtn->setStyleSheet(aBtn->styleSheet() + "background-color: #32CD32;");
    layout->addWidget(aBtn, row + 2, col + 1);
}

QPushButton* VirtualControllerWindow::createButton(const QString& text, const QString& buttonId) {
    QPushButton* button = new QPushButton(text, this);
    button->setMinimumSize(50, 50);
    button->setStyleSheet(
        "QPushButton {"
        "   font-size: 16px;"
        "   font-weight: bold;"
        "   border: 2px solid #333;"
        "   border-radius: 5px;"
        "   background-color: #ddd;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #ff6666;"
        "   border: 3px solid #ff0000;"
        "}"
    );
    
    // Connect press and release events
    connect(button, &QPushButton::pressed, this, [this, buttonId]() {
        statusLabel->setText(QString("Pressed: %1").arg(buttonId.toUpper()));
        emit buttonPressed(buttonId);
    });
    
    connect(button, &QPushButton::released, this, [this, buttonId]() {
        statusLabel->setText(QString("Released: %1").arg(buttonId.toUpper()));
        emit buttonReleased(buttonId);
    });
    
    buttons[buttonId] = button;
    return button;
}
