#ifndef GAMELOGIC_H
#define GAMELOGIC_H

#include "Protocol.h"
#include <array>
#include <deque>
#include <random>

/**
 * @brief Game logic implementation (extracted from snake.cpp)
 * Handles all game rules, collision detection, and state updates
 */
class GameLogic {
public:
    static constexpr int GRID_W = 60;
    static constexpr int GRID_H = 40;
    static constexpr int MAX_PLAYERS = 4;
    
    GameLogic();
    
    /**
     * @brief Initialize a new game with specified number of players
     */
    void init(int playerCount);
    
    /**
     * @brief Apply input commands from players
     */
    void applyInputs(const std::array<Protocol::InputCommand, MAX_PLAYERS>& inputs);
    
    /**
     * @brief Update game state (move snakes, check collisions, etc.)
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
     * @brief Get number of alive players
     */
    int getAliveCount() const;
    
private:
    struct InternalPlayerState {
        int id{};
        bool alive{true};
        Protocol::Direction dir{Protocol::Direction::Right};
        std::deque<Protocol::Vec2> body;
        int score{0};
    };
    
    std::vector<InternalPlayerState> m_players;
    std::vector<Protocol::Vec2> m_food;
    bool m_gameActive{false};
    std::mt19937 m_rng;
    
    void spawnFood();
    void movePlayers();
    void resolveFood();
    void resolveCollisions();
    bool isOpposite(Protocol::Direction a, Protocol::Direction b) const;
    Protocol::Vec2 randomCell();
};

#endif // GAMELOGIC_H
