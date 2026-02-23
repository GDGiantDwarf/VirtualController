#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <optional>
#include <cmath>
#include <algorithm>
#include <array>
#include <sstream>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
#endif

// ============================================================
// Config
// ============================================================

constexpr int MAX_PLAYERS = 4;
constexpr int LAP_GOAL = 3;
constexpr int FINISH_LAPS = LAP_GOAL + 1;
constexpr float MAX_SPEED = 7.5f;
constexpr float ACCELERATION = 0.2f;
constexpr float FRICTION = 0.92f;
constexpr float ROTATION_SPEED = 3.0f;
constexpr float PI = 3.14159f;

enum GameState { LOBBY = 0, ACTIVE = 1, ENDED = 2 };

// ============================================================
// Car Physics
// ============================================================

struct Car {
    int playerId{};
    sf::Vector2f position;
    sf::Vector2f velocity;
    float rotation{0.0f};
    float speed{0.0f};
    int lapsCompleted{0};
    bool finishedRace{false};
    bool lastOnFinishLine{false};
};

// ============================================================
// InputAdapter - reads local keyboard/controller input
// ============================================================

class InputAdapter {
public:
    static void getInputState(int player, float& throttle, float& steer) {
        throttle = 0.0f;
        steer = 0.0f;

        // Keyboard input (player 0)
        if (player == 0) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
                throttle += 1.0f;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
                throttle -= 1.0f;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
                steer += 1.0f;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
                steer -= 1.0f;
            }
        }

        bool keyboardActive = (std::abs(throttle) > 0.01f || std::abs(steer) > 0.01f);

        // Controller input
        const float deadzone = 15.0f;
        if (sf::Joystick::isConnected(player) && (player > 0 || !keyboardActive)) {
            float joyX = sf::Joystick::getAxisPosition(player, sf::Joystick::Axis::X);
            float joyY = sf::Joystick::getAxisPosition(player, sf::Joystick::Axis::Y);
            if (std::abs(joyX) > deadzone) {
                steer = joyX / 100.0f;
            }
            if (std::abs(joyY) > deadzone) {
                throttle = -joyY / 100.0f;
            }
        }
    }

    static int countConnectedControllers() {
        int count = 1;  // Always count keyboard as player 0
        for (int i = 1; i < MAX_PLAYERS; ++i) {
            if (sf::Joystick::isConnected(i)) {
                count++;
            }
        }
        return count;
    }
};

// ============================================================
// Game Logic
// ============================================================

class KartingGame {
private:
    std::vector<Car> cars;
    sf::Image trackImage;
    sf::Texture trackTexture;
    std::unique_ptr<sf::Sprite> trackSprite;
    sf::Image carImageBase;
    std::vector<sf::Texture> carTextures;
    std::vector<std::unique_ptr<sf::Sprite>> carSprites;
    int playerCount{0};
    int connectedPlayers{1};
    GameState gameState{LOBBY};
    sf::Font font;
    std::vector<int> finishOrder;
    
public:
    KartingGame() : trackSprite(nullptr) {
        loadAssets();
    }

    bool loadAssets() {
        // Load track
        if (!trackImage.loadFromFile("track.png")) {
            std::cerr << "Failed to load track.png\n";
            return false;
        }
        if (!trackTexture.loadFromImage(trackImage)) {
            std::cerr << "Failed to load track texture from image\n";
            return false;
        }
        
        sf::Sprite* trackPtr = new sf::Sprite(trackTexture);
        trackSprite.reset(trackPtr);

        // Load car
        if (!carImageBase.loadFromFile("car.png")) {
            std::cerr << "Failed to load car.png\n";
            return false;
        }

        // Create recolored cars
        createPlayerCarTextures();
        
        // Load system font
        if (!font.openFromFile("C:/Windows/Fonts/arial.ttf")) {
            std::cerr << "Failed to load font: C:/Windows/Fonts/arial.ttf\n";
            return false;
        }

        return true;
    }

    void createPlayerCarTextures() {
        sf::Color colors[MAX_PLAYERS] = {
            sf::Color::Red,
            sf::Color::Blue,
            sf::Color::Yellow,
            sf::Color::Magenta
        };

        for (int i = 0; i < MAX_PLAYERS; i++) {
            sf::Image coloredCar = carImageBase;

            for (unsigned int y = 0; y < coloredCar.getSize().y; y++) {
                for (unsigned int x = 0; x < coloredCar.getSize().x; x++) {
                    sf::Color pixel = coloredCar.getPixel({x, y});

                    // Skip fully transparent pixels
                    if (pixel.a < 128) continue;

                    // Skip grass/dark colored areas
                    if (pixel.g > 150 && pixel.r < 120 && pixel.b < 120) continue;

                    // Calculate brightness (0-1 range) from pixel
                    float brightness = (pixel.r + pixel.g + pixel.b) / (3.0f * 255.0f);

                    // Only recolor light pixels (skip very dark areas)
                    if (brightness > 0.2f) {
                        sf::Color newColor = colors[i];
                        newColor.r = static_cast<unsigned char>(newColor.r * brightness);
                        newColor.g = static_cast<unsigned char>(newColor.g * brightness);
                        newColor.b = static_cast<unsigned char>(newColor.b * brightness);
                        newColor.a = pixel.a;
                        coloredCar.setPixel({x, y}, newColor);
                    }
                }
            }

            sf::Texture tex;
            if (!tex.loadFromImage(coloredCar)) {
                std::cerr << "Failed to create car texture for player " << i + 1 << "\n";
                continue;
            }
            carTextures.push_back(tex);
        }
    }

    void initGame(int numPlayers) {
        playerCount = numPlayers;
        cars.clear();
        carSprites.clear();
        finishOrder.clear();

        // Spawn cars at distributed positions on the track
        // These are approximate safe spots on the track
        std::array<sf::Vector2f, 4> spawnPoints = {
            sf::Vector2f(577.0f, 533.0f),
            sf::Vector2f(503.0f, 574.0f),
            sf::Vector2f(431.0f, 533.0f),
            sf::Vector2f(357.0f, 574.0f)
        };

        for (int i = 0; i < numPlayers; i++) {
            Car car;
            car.playerId = i;
            car.position = spawnPoints[i];
            car.rotation = 0.0f;
            car.speed = 0.0f;
            car.lapsCompleted = 0;
            car.finishedRace = false;
            car.lastOnFinishLine = false;
            cars.push_back(car);

            sf::Sprite* ptr = new sf::Sprite(carTextures[i]);
            ptr->setPosition(car.position);
            carSprites.push_back(std::unique_ptr<sf::Sprite>(ptr));
        }

        gameState = ACTIVE;
    }

    void handleInput(int playerId, int direction) {
        if (gameState != ACTIVE) return;
        if (playerId < 0 || playerId >= static_cast<int>(cars.size())) return;

        Car& car = cars[playerId];

        switch (direction) {
            case 0: // Up
                car.speed = std::min(car.speed + ACCELERATION, MAX_SPEED);
                break;
            case 1: // Down
                car.speed = std::max(car.speed - ACCELERATION, -MAX_SPEED * 0.5f);
                break;
            case 2: // Left
                car.rotation -= ROTATION_SPEED;
                break;
            case 3: // Right
                car.rotation += ROTATION_SPEED;
                break;
        }
    }

    void applyInputState(int playerId, float throttle, float steer) {
        if (gameState != ACTIVE) return;
        if (playerId < 0 || playerId >= static_cast<int>(cars.size())) return;

        Car& car = cars[playerId];
        float clampedThrottle = std::clamp(throttle, -1.0f, 1.0f);
        float clampedSteer = std::clamp(steer, -1.0f, 1.0f);

        car.speed = std::clamp(
            car.speed + (ACCELERATION * clampedThrottle),
            -MAX_SPEED * 0.5f,
            MAX_SPEED
        );
        car.rotation += ROTATION_SPEED * clampedSteer;
    }

    void update() {
        if (gameState != ACTIVE) return;

        for (size_t i = 0; i < cars.size(); i++) {
            Car& car = cars[i];
            if (car.finishedRace) continue;

            car.speed *= FRICTION;

            float radians = car.rotation * PI / 180.0f;
            car.velocity.x = car.speed * std::cos(radians);
            car.velocity.y = car.speed * std::sin(radians);

            sf::Vector2f newPos = car.position + car.velocity;

            if (isOnTrack(newPos)) {
                car.position = newPos;
            } else {
                car.speed *= 0.5f;
            }

            checkLapCompletion(car);

            if (car.lapsCompleted >= FINISH_LAPS && !car.finishedRace) {
                car.finishedRace = true;
                finishOrder.push_back(car.playerId);
            }

            // Update sprite
            carSprites[i]->setPosition(car.position);
            carSprites[i]->setRotation(sf::degrees(car.rotation));
            auto bounds = carSprites[i]->getLocalBounds();
            carSprites[i]->setOrigin(sf::Vector2f(bounds.size.x / 2.0f, bounds.size.y / 2.0f));
        }

        // Check if all players finished
        bool allFinished = true;
        for (const auto& car : cars) {
            if (!car.finishedRace) {
                allFinished = false;
                break;
            }
        }
        if (allFinished && cars.size() > 0) {
            gameState = ENDED;
        }
    }

    bool isOnTrack(const sf::Vector2f& pos) {
        int x = static_cast<int>(pos.x);
        int y = static_cast<int>(pos.y);

        if (x < 0 || y < 0 || x >= static_cast<int>(trackImage.getSize().x) ||
            y >= static_cast<int>(trackImage.getSize().y)) {
            return false;
        }

        sf::Color pixel = trackImage.getPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)});

        // Green pixels are grass/boundaries
        if (pixel.g > 150 && pixel.r < 120 && pixel.b < 120) {
            return false;
        }

        return true;
    }

    void checkLapCompletion(Car& car) {
        int checkX = static_cast<int>(car.position.x);
        int checkY = static_cast<int>(car.position.y);

        bool onFinishLine = false;

        for (int dy = -10; dy <= 10; dy++) {
            int y = checkY + dy;
            if (y < 0 || y >= static_cast<int>(trackImage.getSize().y)) continue;

            sf::Color pixel = trackImage.getPixel({static_cast<unsigned int>(checkX), static_cast<unsigned int>(y)});

            // Yellow pixels
            if (pixel.r > 200 && pixel.g > 200 && pixel.b < 100) {
                onFinishLine = true;
                break;
            }
        }

        if (onFinishLine && !car.lastOnFinishLine) {
            car.lapsCompleted++;
        }

        car.lastOnFinishLine = onFinishLine;
    }

    void render(sf::RenderWindow& window) {
        if (gameState == LOBBY) {
            renderLobby(window);
        } else if (gameState == ACTIVE) {
            renderGame(window);
        } else if (gameState == ENDED) {
            renderEndScreen(window);
        }
    }

    void renderGame(sf::RenderWindow& window) {
        window.clear(sf::Color::Black);
        window.draw(*trackSprite);

        for (auto& sprite : carSprites) {
            window.draw(*sprite);
        }

        // Draw HUD - one line per player
        sf::Color playerColors[4] = {
            sf::Color::Red,
            sf::Color::Blue,
            sf::Color::Yellow,
            sf::Color::Magenta
        };

        for (size_t i = 0; i < cars.size(); i++) {
            sf::Text playerHud(font);
            std::ostringstream oss;
            int displayLaps = std::min(cars[i].lapsCompleted, LAP_GOAL);
            oss << "P" << (i + 1) << ": " << displayLaps << "/" << LAP_GOAL;
            playerHud.setString(oss.str());
            playerHud.setCharacterSize(16);
            playerHud.setFillColor(playerColors[i % 4]);
            playerHud.setPosition({10.0f, 10.0f + (i * 20.0f)});
            window.draw(playerHud);
        }

        window.display();
    }

    void renderLobby(sf::RenderWindow& window) {
        window.clear(sf::Color(30, 30, 30));

        sf::Text title(font);
        title.setString("Multiplayer Karting");
        title.setCharacterSize(48);
        title.setFillColor(sf::Color::White);
        title.setPosition({
            (trackSprite->getGlobalBounds().size.x - 400.0f) / 2.0f,
            50.0f
        });
        window.draw(title);

        sf::Text info(font);
        std::ostringstream oss;
        oss << "Use Arrow Keys or WASD to drive\n"
            << "First to " << LAP_GOAL << " laps wins!\n"
            << "Players Connected: " << connectedPlayers << " / " << MAX_PLAYERS << "\n"
            << "Press SPACE to start";
        info.setString(oss.str());
        info.setCharacterSize(20);
        info.setFillColor(sf::Color(200, 200, 200));
        info.setPosition({50.0f, 150.0f});
        window.draw(info);

        sf::Text startText(font);
        startText.setString("Press SPACE to begin");
        startText.setCharacterSize(24);
        startText.setFillColor(sf::Color::Yellow);
        startText.setPosition({
            (trackSprite->getGlobalBounds().size.x - 300.0f) / 2.0f,
            trackSprite->getGlobalBounds().size.y - 100.0f
        });
        window.draw(startText);

        window.display();
    }

    void renderEndScreen(sf::RenderWindow& window) {
        window.clear(sf::Color(30, 30, 30));

        sf::Text gameOver(font);
        gameOver.setString("RACE FINISHED!");
        gameOver.setCharacterSize(48);
        gameOver.setFillColor(sf::Color::Yellow);
        gameOver.setPosition({
            (trackSprite->getGlobalBounds().size.x - 400.0f) / 2.0f,
            50.0f
        });
        window.draw(gameOver);

        sf::Color playerColors[4] = {
            sf::Color::Red,
            sf::Color::Blue,
            sf::Color::Yellow,
            sf::Color::Magenta
        };

        std::string placeBadges[] = {"1st", "2nd", "3rd", "4th"};

        float leaderboardStartY = 150.0f;
        float lineHeight = 50.0f;

        for (size_t i = 0; i < finishOrder.size(); i++) {
            sf::Text placeText(font);
            std::ostringstream oss;
            int playerIdx = finishOrder[i];
            oss << placeBadges[i] << " - Player " << (playerIdx + 1);
            placeText.setString(oss.str());
            placeText.setCharacterSize(28);
            placeText.setFillColor(playerColors[playerIdx % 4]);
            placeText.setPosition({100.0f, leaderboardStartY + (i * lineHeight)});
            window.draw(placeText);
        }

        sf::Text restart(font);
        restart.setString("Press SPACE to return to lobby");
        restart.setCharacterSize(20);
        restart.setFillColor(sf::Color::Green);
        restart.setPosition({
            (trackSprite->getGlobalBounds().size.x - 350.0f) / 2.0f,
            trackSprite->getGlobalBounds().size.y - 80.0f
        });
        window.draw(restart);

        window.display();
    }

    GameState getGameState() const { return gameState; }
    void setGameState(GameState state) { gameState = state; }
    float getTrackWidth() const { return trackSprite->getGlobalBounds().size.x; }
    float getTrackHeight() const { return trackSprite->getGlobalBounds().size.y; }
    int getPlayerCount() const { return playerCount; }
    void setConnectedPlayers(int count) {
        connectedPlayers = std::clamp(count, 1, MAX_PLAYERS);
    }
};

// ============================================================
// Main
// ============================================================

int main(int argc, char* argv[]) {
    std::string serverHost = "127.0.0.1";
    int serverPort = 8765;

    if (argc >= 2) serverHost = argv[1];
    if (argc >= 3) serverPort = std::stoi(argv[2]);

    std::cout << "Connecting to server at " << serverHost << ":" << serverPort << "\n";

    KartingGame game;

    // Create window with track dimensions
    sf::RenderWindow window(
        sf::VideoMode({
            static_cast<unsigned int>(game.getTrackWidth()),
            static_cast<unsigned int>(game.getTrackHeight())
        }),
        "Karting Game"
    );
    window.setFramerateLimit(60);

    int controllerCount = InputAdapter::countConnectedControllers();
    game.setConnectedPlayers(controllerCount);
    sf::Clock controllerCheckClock;

    while (window.isOpen()) {
        std::optional<sf::Event> maybeEvent = window.pollEvent();
        
        while (maybeEvent) {
            const sf::Event& event = maybeEvent.value();
            
            if (event.is<sf::Event::Closed>()) {
                window.close();
            } else if (const auto* keyEvent = event.getIf<sf::Event::KeyPressed>()) {
                if (keyEvent->code == sf::Keyboard::Key::Space) {
                    if (game.getGameState() == LOBBY) {
                        game.initGame(controllerCount);
                    } else if (game.getGameState() == ENDED) {
                        game.setGameState(LOBBY);
                    }
                }
            }
            
            maybeEvent = window.pollEvent();
        }

        if (game.getGameState() == ACTIVE) {
            for (int player = 0; player < game.getPlayerCount(); ++player) {
                float throttle = 0.0f;
                float steer = 0.0f;
                InputAdapter::getInputState(player, throttle, steer);
                game.applyInputState(player, throttle, steer);
            }
            game.update();
        } else if (game.getGameState() == LOBBY) {
            if (controllerCheckClock.getElapsedTime().asMilliseconds() > 500) {
                int newControllerCount = InputAdapter::countConnectedControllers();
                if (newControllerCount != controllerCount) {
                    controllerCount = newControllerCount;
                    game.setConnectedPlayers(controllerCount);
                }
                controllerCheckClock.restart();
            }
        }

        game.render(window);
    }

    return 0;
}
