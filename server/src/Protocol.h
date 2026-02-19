#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>
#include <vector>

/**
 * @brief Shared protocol definitions for client-server communication
 */

namespace Protocol {

// Message types
enum class MessageType {
    CONNECT,        // Client connects
    DISCONNECT,     // Client disconnects
    INPUT,          // Client sends input command
    STATE_UPDATE,   // Server broadcasts game state
    START_GAME,     // Start a new game
    RESET_GAME,     // Reset game to lobby
    MSG_ERROR       // Error message (renamed to avoid Windows ERROR macro)
};

// Direction enumeration (matches game logic)
enum class Direction {
    Up = 0,
    Down = 1,
    Left = 2,
    Right = 3
};

// Game state enumeration
enum class GameStateType {
    LOBBY,   // Waiting for players, before start
    ACTIVE,  // Game is running
    ENDED    // All players dead, waiting for reset
};

// Vector2 position
struct Vec2 {
    int x{};
    int y{};
};

// Player state structure
struct PlayerState {
    int id{};
    bool alive{true};
    Direction dir{Direction::Right};
    std::vector<Vec2> body;
    int score{0};
};

// Game state structure
struct GameState {
    std::vector<PlayerState> players;
    std::vector<Vec2> food;
    bool gameActive{false};
    GameStateType state{GameStateType::LOBBY};
};

// Input command structure
struct InputCommand {
    int playerId{0};
    Direction direction{Direction::Right};
};

// Message structure
struct Message {
    MessageType type;
    int playerId{-1};
    Direction direction{Direction::Right};
    GameState state;
    std::string error;
    int controllerCount{0};  // Number of controllers on the client PC
};

} // namespace Protocol

#endif // PROTOCOL_H
