#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>

int main() {
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Virtual Controller Input Tester");
    window.setFramerateLimit(60);
    
    // Font for displaying text
    sf::Font font;
    // We'll use console output instead of rendering text since we don't have a font file
    
    std::cout << "=== Virtual Controller Input Tester ===" << std::endl;
    std::cout << "Press buttons on the virtual controller..." << std::endl;
    std::cout << std::endl;
    
    sf::Clock clock;
    float updateTimer = 0.0f;
    
    // Track previous values to detect changes
    float lastPovX = 0, lastPovY = 0;
    float lastAxisX = 0, lastAxisY = 0;
    bool lastButtons[32] = {false};
    
    while(window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();
        updateTimer += deltaTime;
        
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            
            if (event->is<sf::Event::KeyPressed>()) {
                auto* keyEvent = event->getIf<sf::Event::KeyPressed>();
                if (keyEvent->code == sf::Keyboard::Key::Escape) {
                    window.close();
                }
            }
        }
        
        if(sf::Joystick::isConnected(0)) {
            // Read all axes
            float povX = sf::Joystick::getAxisPosition(0, sf::Joystick::Axis::PovX);
            float povY = sf::Joystick::getAxisPosition(0, sf::Joystick::Axis::PovY);
            float axisX = sf::Joystick::getAxisPosition(0, sf::Joystick::Axis::X);
            float axisY = sf::Joystick::getAxisPosition(0, sf::Joystick::Axis::Y);
            float axisZ = sf::Joystick::getAxisPosition(0, sf::Joystick::Axis::Z);
            float axisR = sf::Joystick::getAxisPosition(0, sf::Joystick::Axis::R);
            float axisU = sf::Joystick::getAxisPosition(0, sf::Joystick::Axis::U);
            float axisV = sf::Joystick::getAxisPosition(0, sf::Joystick::Axis::V);
            
            // Check for D-Pad changes (POV)
            if(std::abs(povY - lastPovY) > 10) {
                std::cout << "[D-PAD Y] Value: " << std::fixed << std::setprecision(2) << povY;
                if(povY > 50) {
                    std::cout << " -> UP pressed";
                } else if(povY < -50) {
                    std::cout << " -> DOWN pressed";
                } else {
                    std::cout << " -> Released";
                }
                std::cout << std::endl;
                lastPovY = povY;
            }
            
            if(std::abs(povX - lastPovX) > 10) {
                std::cout << "[D-PAD X] Value: " << std::fixed << std::setprecision(2) << povX;
                if(povX > 50) {
                    std::cout << " -> RIGHT pressed";
                } else if(povX < -50) {
                    std::cout << " -> LEFT pressed";
                } else {
                    std::cout << " -> Released";
                }
                std::cout << std::endl;
                lastPovX = povX;
            }
            
            // Check for analog stick changes
            if(std::abs(axisX - lastAxisX) > 10) {
                std::cout << "[LEFT STICK X] Value: " << std::fixed << std::setprecision(2) << axisX << std::endl;
                lastAxisX = axisX;
            }
            
            if(std::abs(axisY - lastAxisY) > 10) {
                std::cout << "[LEFT STICK Y] Value: " << std::fixed << std::setprecision(2) << axisY << std::endl;
                lastAxisY = axisY;
            }
            
            // Check for button presses
            unsigned int buttonCount = sf::Joystick::getButtonCount(0);
            for(unsigned int i = 0; i < buttonCount && i < 32; i++) {
                bool pressed = sf::Joystick::isButtonPressed(0, i);
                if(pressed != lastButtons[i]) {
                    std::cout << "[BUTTON " << i << "] ";
                    
                    // Map common Xbox 360 buttons
                    switch(i) {
                        case 0: std::cout << "(A) "; break;
                        case 1: std::cout << "(B) "; break;
                        case 2: std::cout << "(X) "; break;
                        case 3: std::cout << "(Y) "; break;
                        case 4: std::cout << "(LB) "; break;
                        case 5: std::cout << "(RB) "; break;
                        case 6: std::cout << "(BACK) "; break;
                        case 7: std::cout << "(START) "; break;
                        case 8: std::cout << "(L-STICK) "; break;
                        case 9: std::cout << "(R-STICK) "; break;
                        default: break;
                    }
                    
                    std::cout << (pressed ? "PRESSED" : "RELEASED") << std::endl;
                    lastButtons[i] = pressed;
                }
            }
            
            // Print full status every 2 seconds
            if(updateTimer > 2.0f) {
                std::cout << "\n--- Status Update ---" << std::endl;
                std::cout << "D-Pad: X=" << std::fixed << std::setprecision(1) << povX 
                          << " Y=" << povY << std::endl;
                std::cout << "Left Stick: X=" << axisX << " Y=" << axisY << std::endl;
                std::cout << "Other Axes: Z=" << axisZ << " R=" << axisR 
                          << " U=" << axisU << " V=" << axisV << std::endl;
                
                std::cout << "Buttons pressed: ";
                bool anyPressed = false;
                for(unsigned int i = 0; i < buttonCount && i < 32; i++) {
                    if(sf::Joystick::isButtonPressed(0, i)) {
                        std::cout << i << " ";
                        anyPressed = true;
                    }
                }
                if(!anyPressed) std::cout << "None";
                std::cout << "\n" << std::endl;
                
                updateTimer = 0.0f;
            }
        } else {
            static bool notConnectedLogged = false;
            if(!notConnectedLogged) {
                std::cout << "No joystick connected!" << std::endl;
                notConnectedLogged = true;
            }
        }
        
        // Simple visual feedback
        window.clear(sf::Color(30, 30, 40));
        
        // Draw a simple status indicator
        sf::CircleShape indicator(20);
        indicator.setPosition(sf::Vector2f(10, 10));
        if(sf::Joystick::isConnected(0)) {
            indicator.setFillColor(sf::Color::Green);
        } else {
            indicator.setFillColor(sf::Color::Red);
        }
        window.draw(indicator);
        
        // Draw button indicators
        if(sf::Joystick::isConnected(0)) {
            unsigned int buttonCount = sf::Joystick::getButtonCount(0);
            for(unsigned int i = 0; i < buttonCount && i < 10; i++) {
                sf::CircleShape buttonCircle(15);
                buttonCircle.setPosition(sf::Vector2f(60 + i * 40, 10));
                
                if(sf::Joystick::isButtonPressed(0, i)) {
                    buttonCircle.setFillColor(sf::Color::Yellow);
                } else {
                    buttonCircle.setFillColor(sf::Color(60, 60, 60));
                }
                buttonCircle.setOutlineColor(sf::Color::White);
                buttonCircle.setOutlineThickness(2);
                window.draw(buttonCircle);
            }
            
            // D-Pad visualization
            float povX = sf::Joystick::getAxisPosition(0, sf::Joystick::Axis::PovX);
            float povY = sf::Joystick::getAxisPosition(0, sf::Joystick::Axis::PovY);
            
            sf::RectangleShape dpadCenter(sf::Vector2f(60, 60));
            dpadCenter.setPosition(sf::Vector2f(370, 270));
            dpadCenter.setFillColor(sf::Color(40, 40, 40));
            dpadCenter.setOutlineColor(sf::Color::White);
            dpadCenter.setOutlineThickness(2);
            window.draw(dpadCenter);
            
            // UP
            if(povY > 50) {
                sf::RectangleShape up(sf::Vector2f(30, 20));
                up.setPosition(sf::Vector2f(385, 240));
                up.setFillColor(sf::Color::Cyan);
                window.draw(up);
            }
            // DOWN
            if(povY < -50) {
                sf::RectangleShape down(sf::Vector2f(30, 20));
                down.setPosition(sf::Vector2f(385, 340));
                down.setFillColor(sf::Color::Cyan);
                window.draw(down);
            }
            // LEFT
            if(povX < -50) {
                sf::RectangleShape left(sf::Vector2f(20, 30));
                left.setPosition(sf::Vector2f(340, 285));
                left.setFillColor(sf::Color::Cyan);
                window.draw(left);
            }
            // RIGHT
            if(povX > 50) {
                sf::RectangleShape right(sf::Vector2f(20, 30));
                right.setPosition(sf::Vector2f(440, 285));
                right.setFillColor(sf::Color::Cyan);
                window.draw(right);
            }
            
            // Left stick visualization
            float stickX = sf::Joystick::getAxisPosition(0, sf::Joystick::Axis::X);
            float stickY = sf::Joystick::getAxisPosition(0, sf::Joystick::Axis::Y);
            
            sf::CircleShape stickBase(50);
            stickBase.setPosition(sf::Vector2f(600, 250));
            stickBase.setFillColor(sf::Color(40, 40, 40));
            stickBase.setOutlineColor(sf::Color::White);
            stickBase.setOutlineThickness(2);
            window.draw(stickBase);
            
            sf::CircleShape stickPos(15);
            float stickPosX = 650 + (stickX / 100.0f) * 40;
            float stickPosY = 300 + (stickY / 100.0f) * 40;
            stickPos.setPosition(sf::Vector2f(stickPosX - 15, stickPosY - 15));
            stickPos.setFillColor(sf::Color::Red);
            window.draw(stickPos);
        }
        
        window.display();
    }
    
    std::cout << "\nTester closed." << std::endl;
    return 0;
}