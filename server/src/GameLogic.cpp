#include "GameLogic.h"
#include <algorithm>

GameLogic::GameLogic() : m_rng(std::random_device{}()) {
}

void GameLogic::init(int playerCount) {
    m_players.clear();
    m_food.clear();
    m_gameActive = false; // Start in lobby mode, not active
    m_state = Protocol::GameStateType::LOBBY;
    
    // Clamp player count to valid range (1-4)
    playerCount = std::max(1, std::min(playerCount, MAX_PLAYERS));
    m_playerCount = playerCount;
    
    // Define starting positions based on player count
    // Grid is 60x40
    std::array<Protocol::Vec2, 4> starts;
    std::array<Protocol::Direction, 4> dirs;
    
    switch (playerCount) {
        case 1:
            // Single player at center
            starts[0] = Protocol::Vec2{30, 20};
            dirs[0] = Protocol::Direction::Right;
            break;
        case 2:
            // Two players at opposite corners
            starts[0] = Protocol::Vec2{10, 10};
            dirs[0] = Protocol::Direction::Right;
            starts[1] = Protocol::Vec2{50, 30};
            dirs[1] = Protocol::Direction::Left;
            break;
        case 3:
            // Three players in triangle formation
            starts[0] = Protocol::Vec2{10, 10};
            dirs[0] = Protocol::Direction::Right;
            starts[1] = Protocol::Vec2{50, 10};
            dirs[1] = Protocol::Direction::Left;
            starts[2] = Protocol::Vec2{30, 30};
            dirs[2] = Protocol::Direction::Up;
            break;
        case 4:
        default:
            // Four players at corners
            starts[0] = Protocol::Vec2{10, 10};
            dirs[0] = Protocol::Direction::Right;
            starts[1] = Protocol::Vec2{50, 10};
            dirs[1] = Protocol::Direction::Left;
            starts[2] = Protocol::Vec2{10, 30};
            dirs[2] = Protocol::Direction::Right;
            starts[3] = Protocol::Vec2{50, 30};
            dirs[3] = Protocol::Direction::Left;
            break;
    }
    
    // Create players
    for (int i = 0; i < playerCount; ++i) {
        InternalPlayerState p;
        p.id = i;
        p.dir = dirs[i];
        p.alive = true;
        p.score = 0;
        
        Protocol::Vec2 head = starts[i];
        p.body.push_back(head);
        
        // Add initial body segments
        for (int s = 1; s < 3; ++s) {
            Protocol::Vec2 segment = head;
            switch (p.dir) {
                case Protocol::Direction::Right: segment.x -= s; break;
                case Protocol::Direction::Left:  segment.x += s; break;
                case Protocol::Direction::Up:    segment.y += s; break;
                case Protocol::Direction::Down:  segment.y -= s; break;
            }
            p.body.push_back(segment);
        }
        
        m_players.push_back(p);
        spawnFood();
    }
}

void GameLogic::applyInputs(const std::array<Protocol::InputCommand, MAX_PLAYERS>& inputs) {
    for (auto& p : m_players) {
        if (!p.alive) continue;
        
        const auto& cmd = inputs[p.id];
        if (!isOpposite(p.dir, cmd.direction)) {
            p.dir = cmd.direction;
        }
    }
}

void GameLogic::tick() {
    if (!m_gameActive) return;
    
    movePlayers();
    resolveFood();
    resolveCollisions();
    
    // Check if game should end (all players dead)
    if (getAliveCount() == 0) {
        m_gameActive = false;
        m_state = Protocol::GameStateType::ENDED;
    }
}

Protocol::GameState GameLogic::getState() const {
    Protocol::GameState state;
    state.gameActive = m_gameActive;
    state.state = m_state;
    state.food = m_food;
    
    for (const auto& p : m_players) {
        Protocol::PlayerState ps;
        ps.id = p.id;
        ps.alive = p.alive;
        ps.dir = p.dir;
        ps.score = p.score;
        ps.body = std::vector<Protocol::Vec2>(p.body.begin(), p.body.end());
        state.players.push_back(ps);
    }
    
    return state;
}

bool GameLogic::isGameActive() const {
    return m_gameActive;
}

int GameLogic::getAliveCount() const {
    int count = 0;
    for (const auto& p : m_players) {
        if (p.alive) count++;
    }
    return count;
}

int GameLogic::getPlayerCount() const {
    return m_playerCount;
}

void GameLogic::startGame() {
    m_gameActive = true;
    m_state = Protocol::GameStateType::ACTIVE;
}

void GameLogic::resetGame() {
    // Re-initialize game back to lobby
    init(m_playerCount);
}

void GameLogic::spawnFood() {
    std::uniform_int_distribution<> x(0, GRID_W - 1);
    std::uniform_int_distribution<> y(0, GRID_H - 1);
    m_food.push_back({x(m_rng), y(m_rng)});
}

void GameLogic::movePlayers() {
    for (auto& p : m_players) {
        if (!p.alive) continue;
        
        Protocol::Vec2 head = p.body.front();
        
        switch (p.dir) {
            case Protocol::Direction::Up:    head.y--; break;
            case Protocol::Direction::Down:  head.y++; break;
            case Protocol::Direction::Left:  head.x--; break;
            case Protocol::Direction::Right: head.x++; break;
        }
        
        p.body.push_front(head);
        p.body.pop_back();
    }
}

void GameLogic::resolveFood() {
    for (auto& p : m_players) {
        if (!p.alive) continue;
        
        for (auto& f : m_food) {
            if (p.body.front().x == f.x && p.body.front().y == f.y) {
                p.body.push_back(p.body.back());
                p.score += 10;
                f = randomCell();
            }
        }
    }
}

void GameLogic::resolveCollisions() {
    for (auto& p : m_players) {
        if (!p.alive) continue;
        
        const Protocol::Vec2& h = p.body.front();
        
        // Wall collision
        if (h.x < 0 || h.y < 0 || h.x >= GRID_W || h.y >= GRID_H) {
            p.alive = false;
            continue;
        }
        
        // Snake collision (self and others)
        for (const auto& other : m_players) {
            for (size_t i = (&other == &p ? 1 : 0); i < other.body.size(); ++i) {
                if (h.x == other.body[i].x && h.y == other.body[i].y) {
                    p.alive = false;
                    break;
                }
            }
            if (!p.alive) break;
        }
    }
}

bool GameLogic::isOpposite(Protocol::Direction a, Protocol::Direction b) const {
    return (a == Protocol::Direction::Up && b == Protocol::Direction::Down) ||
           (a == Protocol::Direction::Down && b == Protocol::Direction::Up) ||
           (a == Protocol::Direction::Left && b == Protocol::Direction::Right) ||
           (a == Protocol::Direction::Right && b == Protocol::Direction::Left);
}

Protocol::Vec2 GameLogic::randomCell() {
    std::uniform_int_distribution<> x(0, GRID_W - 1);
    std::uniform_int_distribution<> y(0, GRID_H - 1);
    return {x(m_rng), y(m_rng)};
}
