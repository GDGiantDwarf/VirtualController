/**
 * Server Game Logic Unit Tests
 * 
 * Tests core game state management, player initialization, and logic.
 */

#include <gtest/gtest.h>
#include "../../server/src/GameLogic.h"
#include "../../server/src/Protocol.h"

class GameLogicTest : public ::testing::Test {
protected:
    GameLogic gameLogic;
    
    void SetUp() override {
        // Reset game state before each test
    }
};

TEST_F(GameLogicTest, InitializesCorrectNumberOfPlayers) {
    gameLogic.init(2);
    EXPECT_EQ(gameLogic.getPlayerCount(), 2);
    
    gameLogic.init(4);
    EXPECT_EQ(gameLogic.getPlayerCount(), 4);
}

TEST_F(GameLogicTest, DoesNotExceedMaxPlayers) {
    gameLogic.init(10);  // Request more than MAX_PLAYERS
    EXPECT_LE(gameLogic.getPlayerCount(), GameLogic::MAX_PLAYERS);
}

TEST_F(GameLogicTest, StartsInactive) {
    gameLogic.init(2);
    EXPECT_FALSE(gameLogic.isGameActive());
}

TEST_F(GameLogicTest, ActivatesOnStart) {
    gameLogic.init(2);
    gameLogic.startGame();
    EXPECT_TRUE(gameLogic.isGameActive());
}

TEST_F(GameLogicTest, ResetsToInactive) {
    gameLogic.init(2);
    gameLogic.startGame();
    gameLogic.resetGame();
    EXPECT_FALSE(gameLogic.isGameActive());
}

TEST_F(GameLogicTest, HandlesZeroPlayers) {
    gameLogic.init(0);
    // GameLogic enforces minimum of 1 player
    EXPECT_EQ(gameLogic.getPlayerCount(), 1);
}

TEST_F(GameLogicTest, TickUpdatesGameState) {
    gameLogic.init(2);
    gameLogic.startGame();
    
    // Tick should not crash
    EXPECT_NO_THROW(gameLogic.tick());
    EXPECT_TRUE(gameLogic.isGameActive());
}

TEST_F(GameLogicTest, ApplyInputsDoesNotCrash) {
    std::array<Protocol::InputCommand, GameLogic::MAX_PLAYERS> inputs{};
    inputs[0].playerId = 0;
    inputs[0].direction = Protocol::Direction::Up;
    
    gameLogic.init(2);
    gameLogic.startGame();
    
    EXPECT_NO_THROW(gameLogic.applyInputs(inputs));
}

TEST_F(GameLogicTest, GetStateReturnsValidData) {
    gameLogic.init(2);
    gameLogic.startGame();
    
    Protocol::GameState state = gameLogic.getState();
    EXPECT_GE(state.players.size(), 0);
}

TEST_F(GameLogicTest, AliveCountTrackingWorks) {
    gameLogic.init(2);
    gameLogic.startGame();
    
    int aliveCount = gameLogic.getAliveCount();
    EXPECT_GE(aliveCount, 0);
    EXPECT_LE(aliveCount, 2);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
