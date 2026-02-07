#include "GameLogic.h"
#include <algorithm>

GameLogic::GameLogic() : m_rng(std::random_device{}()) {
}

void GameLogic::init(int playerCount) {
    m_players.clear();
    m_food.clear();
    m_gameActive = true;
    
    // Starting positions for up to 4 players
    static std::array<Protocol::Vec2, 4> starts{
        Protocol::Vec2{10, 10}, 
        Protocol::Vec2{50, 10}, 
        Protocol::Vec2{10, 30}, 
        Protocol::Vec2{50, 30}
    };
    
    static std::array<Protocol::Direction, 4> dirs{
        Protocol::Direction::Right, 
        Protocol::Direction::Left, 
        Protocol::Direction::Right, 
        Protocol::Direction::Left
    };
    
    playerCount = std::min(playerCount, MAX_PLAYERS);
    
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
    }
}

Protocol::GameState GameLogic::getState() const {
    Protocol::GameState state;
    state.gameActive = m_gameActive;
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
