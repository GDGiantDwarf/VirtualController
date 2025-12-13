#ifndef VIRTUALCONTROLLERWINDOW_H
#define VIRTUALCONTROLLERWINDOW_H

#include <QWidget>
#include <QPushButton>
#include <QGridLayout>
#include <QLabel>
#include <QMap>

class VirtualControllerWindow : public QWidget {
    Q_OBJECT
    
public:
    explicit VirtualControllerWindow(QWidget* parent = nullptr);
    
signals:
    void buttonPressed(const QString& buttonName);
    void buttonReleased(const QString& buttonName);
    
private:
    void setupUI();
    QPushButton* createButton(const QString& text, const QString& buttonId);
    
    QMap<QString, QPushButton*> buttons;
    QLabel* statusLabel;
};

#endif // VIRTUALCONTROLLERWINDOW_H