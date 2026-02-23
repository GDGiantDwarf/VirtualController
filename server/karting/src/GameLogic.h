#ifndef GAMELOGIC_H
#define GAMELOGIC_H

#include "Protocol.h"
#include <SFML/Graphics/Image.hpp>
#include <array>
#include <vector>
#include <random>

/**
 * @brief Karting game logic implementation
 * Handles car physics, collision detection, and lap tracking using track image
 */
class GameLogic {
public:
    static constexpr int MAX_PLAYERS = 4;
    static constexpr int LAP_GOAL = 3;
    static constexpr int FINISH_LAPS = LAP_GOAL + 1;
    static constexpr float MAX_SPEED = 10.0f;
    static constexpr float ACCELERATION = 0.2f;
    static constexpr float FRICTION = 0.92f;
    static constexpr float ROTATION_SPEED = 3.0f;
    static constexpr float PI = 3.14159265359f;
    
    GameLogic();
    
    /**
     * @brief Initialize a new game with specified number of players
     */
    void init(int playerCount);
    
    /**
     * @brief Apply input commands from players (throttle, steer)
     */
    void applyInputs(const std::array<Protocol::InputCommand, MAX_PLAYERS>& inputs);
    
    /**
     * @brief Update game state (car physics, collision detection, lap tracking)
     */
    void tick();
    
    /**
     * @brief Get current game state for broadcasting
     */
    Protocol::GameState getState() const;
    
    /**
     * @brief Check if game is active
     */
    bool isGameActive() const;
    
    /**
     * @brief Get number of players who haven't finished
     */
    int getActiveCount() const;
    
    /**
     * @brief Get number of connected players
     */
    int getPlayerCount() const;
    
    /**
     * @brief Manually start the game (from lobby)
     */
    void startGame();
    
    /**
     * @brief Reset game back to lobby state
     */
    void resetGame();
    
private:
    struct InternalCarState {
        int playerId{};
        Protocol::Vec2 position;
        Protocol::Vec2 velocity;
        float rotation{0.0f};
        float speed{0.0f};
        int lapsCompleted{0};
        bool finishedRace{false};
        bool lastOnFinishLine{false};
    };
    
    std::vector<InternalCarState> m_cars;
    std::vector<int> m_finishOrder;
    bool m_gameActive{false};
    int m_playerCount{0};
    Protocol::GameStateType m_state{Protocol::GameStateType::LOBBY};
    
    // Track image for collision detection
    sf::Image m_trackImage;
    
    bool isOnTrack(const Protocol::Vec2& pos) const;
    void checkLapCompletion(InternalCarState& car);
    void spawnCars(int playerCount);
};

#endif // GAMELOGIC_H
