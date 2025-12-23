#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <vector>
#include <array>
#include <deque>
#include <random>
#include <cmath>

// ============================================================
// Config
// ============================================================

constexpr int GRID_SIZE = 20;
constexpr int GRID_W = 60;
constexpr int GRID_H = 40;
constexpr float TICK_TIME = 0.12f;
constexpr int MAX_PLAYERS = 4;

// ============================================================
// Shared types
// ============================================================

enum class Direction { Up, Down, Left, Right };

struct Vec2 {
    int x{};
    int y{};
    bool operator==(const Vec2& o) const { return x == o.x && y == o.y; }
};

// ============================================================
// PlayerState
// ============================================================

struct PlayerState {
    int id{};
    bool alive{true};
    Direction dir{Direction::Right};
    std::deque<Vec2> body;
    int score{0};
};

// ============================================================
// Input command
// ============================================================

struct PlayerCommand {
    bool hasDirection{false};
    Direction newDirection{};
};

// ============================================================
// GameRules
// ============================================================

class GameRules {
public:
    std::vector<PlayerState> players;
    std::vector<Vec2> food;

    void init(int count) {
        players.clear();
        food.clear();

        static std::array<Vec2, 4> starts{
            Vec2{10, 10}, Vec2{50, 10}, Vec2{10, 30}, Vec2{50, 30}
        };

        static std::array<Direction, 4> dirs{
            Direction::Right, Direction::Left, Direction::Right, Direction::Left
        };

        for (int i = 0; i < count; ++i) {
            PlayerState p;
            p.id = i;
            p.dir = dirs[i];

            Vec2 head = starts[i];
            p.body.push_back(head);

            for (int s = 1; s < 3; ++s) {
                Vec2 segment = head;
                switch (p.dir) {
                    case Direction::Right: segment.x -= s; break;
                    case Direction::Left:  segment.x += s; break;
                    case Direction::Up:    segment.y += s; break;
                    case Direction::Down:  segment.y -= s; break;
                }
                p.body.push_back(segment);
            }

            players.push_back(p);
            spawnFood();
        }
    }

    void applyCommands(const std::array<PlayerCommand, MAX_PLAYERS>& cmds) {
        for (auto& p : players) {
            if (!p.alive) continue;
            const auto& c = cmds[p.id];
            if (!c.hasDirection) continue;
            if (!isOpposite(p.dir, c.newDirection))
                p.dir = c.newDirection;
        }
    }

    void tick() {
        movePlayers();
        resolveFood();
        resolveCollisions();
    }

private:
    std::mt19937 rng{std::random_device{}()};

    void spawnFood() {
        std::uniform_int_distribution<> x(0, GRID_W - 1);
        std::uniform_int_distribution<> y(0, GRID_H - 1);
        food.push_back({x(rng), y(rng)});
    }

    void movePlayers() {
        for (auto& p : players) {
            if (!p.alive) continue;
            Vec2 head = p.body.front();

            switch (p.dir) {
                case Direction::Up:    head.y--; break;
                case Direction::Down:  head.y++; break;
                case Direction::Left:  head.x--; break;
                case Direction::Right: head.x++; break;
            }

            p.body.push_front(head);
            p.body.pop_back();
        }
    }

    void resolveFood() {
        for (auto& p : players) {
            if (!p.alive) continue;
            for (auto& f : food) {
                if (p.body.front() == f) {
                    p.body.push_back(p.body.back());
                    p.score += 10;
                    f = randomCell();
                }
            }
        }
    }

    void resolveCollisions() {
        for (auto& p : players) {
            if (!p.alive) continue;
            const Vec2& h = p.body.front();

            if (h.x < 0 || h.y < 0 || h.x >= GRID_W || h.y >= GRID_H) {
                p.alive = false;
                continue;
            }

            for (const auto& o : players) {
                for (size_t i = (&o == &p ? 1 : 0); i < o.body.size(); ++i) {
                    if (h == o.body[i]) {
                        p.alive = false;
                        break;
                    }
                }
            }
        }
    }

    bool isOpposite(Direction a, Direction b) {
        return (a == Direction::Up && b == Direction::Down) ||
               (a == Direction::Down && b == Direction::Up) ||
               (a == Direction::Left && b == Direction::Right) ||
               (a == Direction::Right && b == Direction::Left);
    }

    Vec2 randomCell() {
        std::uniform_int_distribution<> x(0, GRID_W - 1);
        std::uniform_int_distribution<> y(0, GRID_H - 1);
        return {x(rng), y(rng)};
    }
};

// ============================================================
// InputAdapter
// ============================================================

class InputAdapter {
public:
    static PlayerCommand readKeyboard(int player) {
        PlayerCommand cmd{};
        if (player != 0) return cmd;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
            cmd = {true, Direction::Up};
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
            cmd = {true, Direction::Down};
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
            cmd = {true, Direction::Left};
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
            cmd = {true, Direction::Right};

        return cmd;
    }

    static PlayerCommand readController(int player) {
        PlayerCommand cmd{};
        if (!sf::Joystick::isConnected(player)) return cmd;

        float x = sf::Joystick::getAxisPosition(player, sf::Joystick::Axis::X);
        float y = sf::Joystick::getAxisPosition(player, sf::Joystick::Axis::Y);

        if (std::abs(x) > std::abs(y)) {
            if (x > 50) cmd = {true, Direction::Right};
            else if (x < -50) cmd = {true, Direction::Left};
        } else {
            if (y > 50) cmd = {true, Direction::Down};
            else if (y < -50) cmd = {true, Direction::Up};
        }

        return cmd;
    }
};

// ============================================================
// Main (client demo)
// ============================================================

int main() {
    sf::RenderWindow window(
        sf::VideoMode({GRID_W * GRID_SIZE, GRID_H * GRID_SIZE}),
        "Multiplayer Snake Demo"
    );
    window.setFramerateLimit(60);

    GameRules game;
    game.init(4);

    sf::Clock clock;
    float accumulator = 0.f;

    static std::array<sf::Color, 4> colors{
        sf::Color::Green,
        sf::Color::Blue,
        sf::Color::Red,
        sf::Color::Yellow
    };

    while (window.isOpen()) {
        while (auto e = window.pollEvent())
            if (e->is<sf::Event::Closed>())
                window.close();

        accumulator += clock.restart().asSeconds();

        if (accumulator >= TICK_TIME) {
            accumulator = 0.f;

            std::array<PlayerCommand, MAX_PLAYERS> cmds{};
            for (int i = 0; i < MAX_PLAYERS; ++i) {
                cmds[i] = InputAdapter::readKeyboard(i);
                if (!cmds[i].hasDirection)
                    cmds[i] = InputAdapter::readController(i);
            }

            game.applyCommands(cmds);
            game.tick();
        }

        window.clear(sf::Color(30, 30, 30));

        sf::RectangleShape cell(
            sf::Vector2f(GRID_SIZE - 2.f, GRID_SIZE - 2.f)
        );

        for (const auto& f : game.food) {
            cell.setFillColor(sf::Color::Red);
            cell.setPosition(sf::Vector2f(
                f.x * GRID_SIZE + 1.f,
                f.y * GRID_SIZE + 1.f
            ));
            window.draw(cell);
        }

        for (const auto& p : game.players) {
            if (!p.alive) continue;
            for (size_t i = 0; i < p.body.size(); ++i) {
                cell.setFillColor(i == 0 ? colors[p.id] : sf::Color(120,120,120));
                cell.setPosition(sf::Vector2f(
                    p.body[i].x * GRID_SIZE + 1.f,
                    p.body[i].y * GRID_SIZE + 1.f
                ));
                window.draw(cell);
            }
        }

        window.display();
    }
}