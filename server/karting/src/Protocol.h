#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>
#include <vector>

/**
 * @brief Shared protocol definitions for client-server communication (Karting)
 */

namespace Protocol {

// Message types
enum class MessageType {
    CONNECT,        // Client connects
    DISCONNECT,     // Client disconnects
    INPUT,          // Client sends input command (throttle, steer)
    STATE_UPDATE,   // Server broadcasts game state
    START_GAME,     // Start a new game
    RESET_GAME,     // Reset game to lobby
    MSG_ERROR       // Error message (renamed to avoid Windows ERROR macro)
};

// Game state enumeration
enum class GameStateType {
    LOBBY,   // Waiting for players, before start
    ACTIVE,  // Game is running
    ENDED    // Race finished
};

// Vector2 position (float for karting)
struct Vec2 {
    float x{};
    float y{};
};

// Car state structure (replaces PlayerState for karting)
struct CarState {
    int playerId{};
    Vec2 position;
    Vec2 velocity;
    float rotation{0.0f};
    float speed{0.0f};
    int lapsCompleted{0};
    bool finishedRace{false};
};

// Game state structure
struct GameState {
    std::vector<CarState> cars;
    std::vector<int> finishOrder;  // Player IDs in finish order
    bool gameActive{false};
    GameStateType state{GameStateType::LOBBY};
};

// Input command structure (throttle and steering for karting)
struct InputCommand {
    int playerId{0};
    float throttle{0.0f};  // -1.0 to 1.0
    float steer{0.0f};     // -1.0 to 1.0
};

// Message structure
struct Message {
    MessageType type;
    int playerId{-1};
    float throttle{0.0f};
    float steer{0.0f};
    GameState state;
    std::string error;
    int controllerCount{0};  // Number of controllers on the client PC
};

} // namespace Protocol

#endif // PROTOCOL_H
