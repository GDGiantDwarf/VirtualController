#include "GameLogic.h"
#include <iostream>
#include <cmath>
#include <algorithm>

GameLogic::GameLogic() {
    // Load track image for collision detection (headless - no window needed)
    if (!m_trackImage.loadFromFile("assets/track.png")) {
        std::cerr << "WARNING: Failed to load assets/track.png for collision detection" << std::endl;
        std::cerr << "Server will continue but collision detection may not work properly" << std::endl;
    } else {
        std::cout << "Loaded track image: " << m_trackImage.getSize().x << "x" 
                  << m_trackImage.getSize().y << std::endl;
    }
}

void GameLogic::init(int playerCount) {
    if (playerCount < 1 || playerCount > MAX_PLAYERS) {
        playerCount = std::clamp(playerCount, 1, MAX_PLAYERS);
    }
    
    m_playerCount = playerCount;
    m_gameActive = false;
    m_state = Protocol::GameStateType::LOBBY;
    m_cars.clear();
    m_finishOrder.clear();
    
    // Spawn cars in lobby (like snake spawns players)
    spawnCars(m_playerCount);
}

void GameLogic::startGame() {
    if (m_state != Protocol::GameStateType::LOBBY) return;
    
    // Don't respawn cars - they already exist from init()
    // Just reset their state to starting positions
    for (auto& car : m_cars) {
        car.speed = 0.0f;
        car.velocity = {0.0f, 0.0f};
        car.lapsCompleted = 0;
        car.finishedRace = false;
        car.lastOnFinishLine = false;
    }
    
    m_gameActive = true;
    m_state = Protocol::GameStateType::ACTIVE;
    m_finishOrder.clear();
    
    std::cout << "Game started with " << m_playerCount << " players" << std::endl;
}

void GameLogic::resetGame() {
    m_gameActive = false;
    m_state = Protocol::GameStateType::LOBBY;
    m_cars.clear();
    m_finishOrder.clear();
}

void GameLogic::spawnCars(int playerCount) {
    m_cars.clear();
    
    // Spawn points matching the old karting game
    std::array<Protocol::Vec2, 4> spawnPoints = {
        Protocol::Vec2{577.0f, 533.0f},
        Protocol::Vec2{503.0f, 574.0f},
        Protocol::Vec2{431.0f, 533.0f},
        Protocol::Vec2{357.0f, 574.0f}
    };
    
    for (int i = 0; i < playerCount; ++i) {
        InternalCarState car;
        car.playerId = i;
        car.position = spawnPoints[i];
        car.rotation = 0.0f;
        car.speed = 0.0f;
        car.velocity = {0.0f, 0.0f};
        car.lapsCompleted = 0;
        car.finishedRace = false;
        car.lastOnFinishLine = false;
        m_cars.push_back(car);
    }
}

void GameLogic::applyInputs(const std::array<Protocol::InputCommand, MAX_PLAYERS>& inputs) {
    if (!m_gameActive) return;
    
    for (size_t i = 0; i < m_cars.size(); ++i) {
        if (m_cars[i].finishedRace) continue;
        
        const auto& input = inputs[i];
        auto& car = m_cars[i];
        
        // Apply throttle
        float clampedThrottle = std::clamp(input.throttle, -1.0f, 1.0f);
        car.speed = std::clamp(
            car.speed + (ACCELERATION * clampedThrottle),
            -MAX_SPEED * 0.5f,
            MAX_SPEED
        );
        
        // Apply steering
        float clampedSteer = std::clamp(input.steer, -1.0f, 1.0f);
        car.rotation += ROTATION_SPEED * clampedSteer;
    }
}

void GameLogic::tick() {
    if (!m_gameActive) return;
    
    for (auto& car : m_cars) {
        if (car.finishedRace) continue;
        
        // Apply friction
        car.speed *= FRICTION;
        
        // Calculate velocity from rotation and speed
        float radians = car.rotation * PI / 180.0f;
        car.velocity.x = car.speed * std::cos(radians);
        car.velocity.y = car.speed * std::sin(radians);
        
        // Try to move
        Protocol::Vec2 newPos = {
            car.position.x + car.velocity.x,
            car.position.y + car.velocity.y
        };
        
        // Check if new position is on track
        if (isOnTrack(newPos)) {
            car.position = newPos;
        } else {
            // Hit grass - slow down
            car.speed *= 0.5f;
        }
        
        // Check lap completion
        checkLapCompletion(car);
        
        // Check if finished race
        if (car.lapsCompleted >= FINISH_LAPS && !car.finishedRace) {
            car.finishedRace = true;
            m_finishOrder.push_back(car.playerId);
            std::cout << "Player " << car.playerId << " finished in place " 
                      << m_finishOrder.size() << std::endl;
        }
    }
    
    // Check if all players finished
    bool allFinished = true;
    for (const auto& car : m_cars) {
        if (!car.finishedRace) {
            allFinished = false;
            break;
        }
    }
    
    if (allFinished && !m_cars.empty()) {
        m_gameActive = false;
        m_state = Protocol::GameStateType::ENDED;
        std::cout << "Race ended!" << std::endl;
    }
}

bool GameLogic::isOnTrack(const Protocol::Vec2& pos) const {
    int x = static_cast<int>(pos.x);
    int y = static_cast<int>(pos.y);
    
    // Check bounds
    if (x < 0 || y < 0 || 
        x >= static_cast<int>(m_trackImage.getSize().x) ||
        y >= static_cast<int>(m_trackImage.getSize().y)) {
        return false;
    }
    
    // Check pixel color
    sf::Color pixel = m_trackImage.getPixel({static_cast<unsigned int>(x), 
                                              static_cast<unsigned int>(y)});
    
    // Green pixels are grass/boundaries (R < 120, G > 150, B < 120)
    if (pixel.g > 150 && pixel.r < 120 && pixel.b < 120) {
        return false;
    }
    
    return true;
}

void GameLogic::checkLapCompletion(InternalCarState& car) {
    int checkX = static_cast<int>(car.position.x);
    int checkY = static_cast<int>(car.position.y);
    
    bool onFinishLine = false;
    
    // Check pixels around the car position for yellow finish line
    for (int dy = -10; dy <= 10; ++dy) {
        int y = checkY + dy;
        if (y < 0 || y >= static_cast<int>(m_trackImage.getSize().y)) continue;
        
        if (checkX < 0 || checkX >= static_cast<int>(m_trackImage.getSize().x)) continue;
        
        sf::Color pixel = m_trackImage.getPixel({static_cast<unsigned int>(checkX), 
                                                  static_cast<unsigned int>(y)});
        
        // Yellow pixels indicate finish line (R > 200, G > 200, B < 100)
        if (pixel.r > 200 && pixel.g > 200 && pixel.b < 100) {
            onFinishLine = true;
            break;
        }
    }
    
    // Detect crossing (edge detection: wasn't on line, now is)
    if (onFinishLine && !car.lastOnFinishLine) {
        car.lapsCompleted++;
        std::cout << "Player " << car.playerId << " completed lap " 
                  << car.lapsCompleted << std::endl;
    }
    
    car.lastOnFinishLine = onFinishLine;
}

Protocol::GameState GameLogic::getState() const {
    Protocol::GameState state;
    state.gameActive = m_gameActive;
    state.state = m_state;
    state.finishOrder = m_finishOrder;
    
    // Convert internal car states to protocol car states
    for (const auto& car : m_cars) {
        Protocol::CarState carState;
        carState.playerId = car.playerId;
        carState.position = car.position;
        carState.velocity = car.velocity;
        carState.rotation = car.rotation;
        carState.speed = car.speed;
        carState.lapsCompleted = car.lapsCompleted;
        carState.finishedRace = car.finishedRace;
        state.cars.push_back(carState);
    }
    
    return state;
}

bool GameLogic::isGameActive() const {
    return m_gameActive;
}

int GameLogic::getActiveCount() const {
    int count = 0;
    for (const auto& car : m_cars) {
        if (!car.finishedRace) ++count;
    }
    return count;
}

int GameLogic::getPlayerCount() const {
    return m_playerCount;
}
