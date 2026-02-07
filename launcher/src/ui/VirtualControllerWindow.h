#ifndef VIRTUALCONTROLLERWINDOW_H
#define VIRTUALCONTROLLERWINDOW_H

#include <QWidget>
#include <QPushButton>
#include <QGridLayout>
#include <QLabel>
#include <QMap>
#include <QTimer>

/**
 * @brief Widget de stick analogique interactif
 */
class AnalogStickWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit AnalogStickWidget(QWidget* parent = nullptr);
    
    void reset();
    QPointF getPosition() const { return m_position; }
    
signals:
    void positionChanged(short x, short y);
    
protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    
private:
    void updatePosition(const QPointF& pos);
    
    QPointF m_position; // Normalized -1.0 to 1.0
    bool m_dragging;
    static constexpr int STICK_RADIUS = 60;
    static constexpr int KNOB_RADIUS = 20;
};

/**
 * @brief Fenêtre de contrôleur virtuel avec support complet
 */
class VirtualControllerWindow : public QWidget {
    Q_OBJECT
    
public:
    explicit VirtualControllerWindow(int controllerId = 1, QWidget* parent = nullptr);
    
signals:
    void buttonPressed(const QString& buttonName);
    void buttonReleased(const QString& buttonName);
    void analogStickMoved(short x, short y);
    
private:
    void setupUI();
    QPushButton* createButton(const QString& text, const QString& buttonId);
    void createDPadWithDiagonals(QGridLayout* layout, int row, int col);
    void createActionButtons(QGridLayout* layout, int row, int col);
    
    QMap<QString, QPushButton*> buttons;
    AnalogStickWidget* analogStick;
    QLabel* statusLabel;
    QLabel* controllerIdLabel;
    int m_controllerId;
};

#endif // VIRTUALCONTROLLERWINDOW_H
