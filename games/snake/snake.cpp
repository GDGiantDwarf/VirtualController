#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iostream>

const unsigned int WINDOW_WIDTH = 800;
const unsigned int WINDOW_HEIGHT = 600;
const int GRID_SIZE = 20;
const float MOVE_DELAY = 0.15f;

enum Direction { UP, DOWN, LEFT, RIGHT };

struct SnakeSegment {
    sf::Vector2i position;
};

class Snake {
public:
    std::vector<SnakeSegment> segments;
    Direction direction;
    
    Snake() {
        direction = RIGHT;
        segments.push_back({{10, 10}});
        segments.push_back({{9, 10}});
        segments.push_back({{8, 10}});
    }
    
    void move() {
        sf::Vector2i newHead = segments[0].position;
        
        switch(direction) {
            case UP: newHead.y--; break;
            case DOWN: newHead.y++; break;
            case LEFT: newHead.x--; break;
            case RIGHT: newHead.x++; break;
        }
        
        segments.insert(segments.begin(), {newHead});
        segments.pop_back();
    }
    
    void grow() {
        sf::Vector2i tail = segments.back().position;
        segments.push_back({tail});
    }
    
    bool checkCollision() {
        sf::Vector2i head = segments[0].position;
        
        // Check wall collision
        if(head.x < 0 || head.x >= static_cast<int>(WINDOW_WIDTH/GRID_SIZE) ||
           head.y < 0 || head.y >= static_cast<int>(WINDOW_HEIGHT/GRID_SIZE)) {
            return true;
        }
        
        // Check self collision
        for(size_t i = 1; i < segments.size(); i++) {
            if(segments[i].position == head) {
                return true;
            }
        }
        
        return false;
    }
    
    void changeDirection(Direction newDir) {
        // Prevent 180 degree turns
        if((direction == UP && newDir != DOWN) ||
           (direction == DOWN && newDir != UP) ||
           (direction == LEFT && newDir != RIGHT) ||
           (direction == RIGHT && newDir != LEFT)) {
            direction = newDir;
        }
    }
};

class Food {
public:
    sf::Vector2i position;
    
    Food() {
        spawn();
    }
    
    void spawn() {
        position.x = rand() % (WINDOW_WIDTH / GRID_SIZE);
        position.y = rand() % (WINDOW_HEIGHT / GRID_SIZE);
    }
};

int main() {
    srand(static_cast<unsigned int>(time(nullptr)));
    
    sf::RenderWindow window(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), "Snake - Virtual Controller Test");
    window.setFramerateLimit(60);
    
    Snake snake;
    Food food;
    
    float moveTimer = 0.0f;
    sf::Clock clock;
    int score = 0;
    bool gameOver = false;
    
    std::cout << "=== Snake Game with Virtual Controller ===" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  Keyboard: Arrow keys or WASD" << std::endl;
    std::cout << "  Virtual Controller: D-Pad buttons" << std::endl;
    std::cout << "  R: Restart after game over" << std::endl;
    std::cout << std::endl;
    
    // Check for gamepad
    sf::Joystick::update();
    if(sf::Joystick::isConnected(0)) {
        std::cout << "✓ Gamepad detected!" << std::endl;
        std::cout << "  Buttons: " << sf::Joystick::getButtonCount(0) << std::endl;
    } else {
        std::cout << "✗ No gamepad detected" << std::endl;
    }
    std::cout << std::endl;
    
    // CRITICAL FIX: Use a separate timer that accumulates deltaTime
    float timeSinceLastInput = 999.0f; // Start high so first input works immediately
    
    while(window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();
        timeSinceLastInput += deltaTime; // Accumulate time
        
        // Event handling
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            
            // Keyboard controls
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (!gameOver) {
                    if (keyPressed->code == sf::Keyboard::Key::Up || keyPressed->code == sf::Keyboard::Key::W) {
                        snake.changeDirection(UP);
                        std::cout << "[KEYBOARD] UP" << std::endl;
                    }
                    else if (keyPressed->code == sf::Keyboard::Key::Down || keyPressed->code == sf::Keyboard::Key::S) {
                        snake.changeDirection(DOWN);
                        std::cout << "[KEYBOARD] DOWN" << std::endl;
                    }
                    else if (keyPressed->code == sf::Keyboard::Key::Left || keyPressed->code == sf::Keyboard::Key::A) {
                        snake.changeDirection(LEFT);
                        std::cout << "[KEYBOARD] LEFT" << std::endl;
                    }
                    else if (keyPressed->code == sf::Keyboard::Key::Right || keyPressed->code == sf::Keyboard::Key::D) {
                        snake.changeDirection(RIGHT);
                        std::cout << "[KEYBOARD] RIGHT" << std::endl;
                    }
                }
                
                // Restart game
                if (gameOver && keyPressed->code == sf::Keyboard::Key::R) {
                    snake = Snake();
                    food.spawn();
                    score = 0;
                    gameOver = false;
                    std::cout << "\n=== GAME RESTARTED ===" << std::endl;
                }
            }
        }
        
        // Gamepad controls - poll continuously
        if(!gameOver && sf::Joystick::isConnected(0)) {
            float povX = sf::Joystick::getAxisPosition(0, sf::Joystick::Axis::PovX);
            float povY = sf::Joystick::getAxisPosition(0, sf::Joystick::Axis::PovY);
            
            // Debug: show current POV values (commented out to reduce spam)
            // static float debugTimer = 0;
            // debugTimer += deltaTime;
            // if(debugTimer > 0.5f) {
            //     std::cout << "POV: X=" << povX << " Y=" << povY << std::endl;
            //     debugTimer = 0;
            // }
            
            // THE FIX: Check input delay with accumulated time
            // Allow input every 0.15 seconds to prevent accidental double-taps
            if(timeSinceLastInput > 0.15f) {
                bool inputDetected = false;
                
                // Check D-Pad (POV)
                if(povY > 50) {
                    snake.changeDirection(UP);
                    std::cout << "[DPAD] UP (povY=" << povY << ")" << std::endl;
                    timeSinceLastInput = 0.0f;
                    inputDetected = true;
                }
                else if(povY < -50) {
                    snake.changeDirection(DOWN);
                    std::cout << "[DPAD] DOWN (povY=" << povY << ")" << std::endl;
                    timeSinceLastInput = 0.0f;
                    inputDetected = true;
                }
                else if(povX < -50) {
                    snake.changeDirection(LEFT);
                    std::cout << "[DPAD] LEFT (povX=" << povX << ")" << std::endl;
                    timeSinceLastInput = 0.0f;
                    inputDetected = true;
                }
                else if(povX > 50) {
                    snake.changeDirection(RIGHT);
                    std::cout << "[DPAD] RIGHT (povX=" << povX << ")" << std::endl;
                    timeSinceLastInput = 0.0f;
                    inputDetected = true;
                }
                
                // Alternative: Check analog stick as fallback
                if(!inputDetected) {
                    float axisX = sf::Joystick::getAxisPosition(0, sf::Joystick::Axis::X);
                    float axisY = sf::Joystick::getAxisPosition(0, sf::Joystick::Axis::Y);
                    float deadzone = 50.0f;
                    
                    if(axisY < -deadzone) {
                        snake.changeDirection(UP);
                        std::cout << "[STICK] UP (axisY=" << axisY << ")" << std::endl;
                        timeSinceLastInput = 0.0f;
                    }
                    else if(axisY > deadzone) {
                        snake.changeDirection(DOWN);
                        std::cout << "[STICK] DOWN (axisY=" << axisY << ")" << std::endl;
                        timeSinceLastInput = 0.0f;
                    }
                    else if(axisX < -deadzone) {
                        snake.changeDirection(LEFT);
                        std::cout << "[STICK] LEFT (axisX=" << axisX << ")" << std::endl;
                        timeSinceLastInput = 0.0f;
                    }
                    else if(axisX > deadzone) {
                        snake.changeDirection(RIGHT);
                        std::cout << "[STICK] RIGHT (axisX=" << axisX << ")" << std::endl;
                        timeSinceLastInput = 0.0f;
                    }
                }
            }
            
            // Button A to restart
            if(gameOver && sf::Joystick::isButtonPressed(0, 0)) {
                snake = Snake();
                food.spawn();
                score = 0;
                gameOver = false;
                std::cout << "\n=== GAME RESTARTED (Button A) ===" << std::endl;
            }
        }
        
        // Game logic
        if(!gameOver) {
            moveTimer += deltaTime;
            
            if(moveTimer >= MOVE_DELAY) {
                moveTimer = 0.0f;
                snake.move();
                
                // Check food collision
                if(snake.segments[0].position == food.position) {
                    snake.grow();
                    food.spawn();
                    score += 10;
                    std::cout << "🍎 Score: " << score << std::endl;
                }
                
                // Check game over
                if(snake.checkCollision()) {
                    gameOver = true;
                    std::cout << "\n💀 GAME OVER! Final score: " << score << std::endl;
                    std::cout << "Press R or Button A to restart" << std::endl;
                }
            }
        }
        
        // Render
        window.clear(sf::Color(30, 30, 30));
        
        // Draw food
        sf::RectangleShape foodShape(sf::Vector2f(GRID_SIZE - 2.f, GRID_SIZE - 2.f));
        foodShape.setFillColor(sf::Color::Red);
        foodShape.setPosition(sf::Vector2f(food.position.x * GRID_SIZE + 1.f, 
                                           food.position.y * GRID_SIZE + 1.f));
        window.draw(foodShape);
        
        // Draw snake
        for(size_t i = 0; i < snake.segments.size(); i++) {
            sf::RectangleShape segment(sf::Vector2f(GRID_SIZE - 2.f, GRID_SIZE - 2.f));
            segment.setFillColor(i == 0 ? sf::Color::Green : sf::Color(0, 200, 0));
            segment.setPosition(sf::Vector2f(snake.segments[i].position.x * GRID_SIZE + 1.f,
                                            snake.segments[i].position.y * GRID_SIZE + 1.f));
            window.draw(segment);
        }
        
        // Draw game over text (simple colored rectangle)
        if(gameOver) {
            sf::RectangleShape overlay(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
            overlay.setFillColor(sf::Color(0, 0, 0, 150));
            window.draw(overlay);
            
            sf::RectangleShape gameOverBox(sf::Vector2f(400, 100));
            gameOverBox.setPosition(sf::Vector2f(200, 250));
            gameOverBox.setFillColor(sf::Color(200, 50, 50));
            window.draw(gameOverBox);
        }
        
        window.display();
    }
    
    std::cout << "\nGame closed. Thanks for playing!" << std::endl;
    return 0;
}