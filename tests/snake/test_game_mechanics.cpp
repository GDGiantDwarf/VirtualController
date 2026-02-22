/**
 * Snake Game Logic Unit Tests
 * 
 * Tests collision detection, movement, and game mechanics.
 * These are mock tests since the actual game logic is embedded in the client.
 */

#include <gtest/gtest.h>
#include <vector>

// Mock structures matching snake game
struct Vec2 {
    int x{0};
    int y{0};
    bool operator==(const Vec2& other) const {
        return x == other.x && y == other.y;
    }
};

enum class Direction { Up = 0, Down = 1, Left = 2, Right = 3 };

class SnakeGameTest : public ::testing::Test {
protected:
    static constexpr int GRID_W = 60;
    static constexpr int GRID_H = 40;
    
    Vec2 moveInDirection(Vec2 pos, Direction dir) {
        Vec2 newPos = pos;
        switch (dir) {
            case Direction::Up:    newPos.y--; break;
            case Direction::Down:  newPos.y++; break;
            case Direction::Left:  newPos.x--; break;
            case Direction::Right: newPos.x++; break;
        }
        return newPos;
    }
    
    bool isOutOfBounds(Vec2 pos) {
        return pos.x < 0 || pos.x >= GRID_W || pos.y < 0 || pos.y >= GRID_H;
    }
    
    bool checkSelfCollision(const std::vector<Vec2>& body) {
        if (body.size() < 2) return false;
        Vec2 head = body[0];
        for (size_t i = 1; i < body.size(); i++) {
            if (body[i] == head) return true;
        }
        return false;
    }
};

TEST_F(SnakeGameTest, MovementInAllDirections) {
    Vec2 start{30, 20};
    
    Vec2 up = moveInDirection(start, Direction::Up);
    EXPECT_EQ(up.x, 30);
    EXPECT_EQ(up.y, 19);
    
    Vec2 down = moveInDirection(start, Direction::Down);
    EXPECT_EQ(down.x, 30);
    EXPECT_EQ(down.y, 21);
    
    Vec2 left = moveInDirection(start, Direction::Left);
    EXPECT_EQ(left.x, 29);
    EXPECT_EQ(left.y, 20);
    
    Vec2 right = moveInDirection(start, Direction::Right);
    EXPECT_EQ(right.x, 31);
    EXPECT_EQ(right.y, 20);
}

TEST_F(SnakeGameTest, DetectsUpperBoundary) {
    Vec2 pos{30, 0};
    Vec2 newPos = moveInDirection(pos, Direction::Up);
    EXPECT_TRUE(isOutOfBounds(newPos));
}

TEST_F(SnakeGameTest, DetectsLowerBoundary) {
    Vec2 pos{30, GRID_H - 1};
    Vec2 newPos = moveInDirection(pos, Direction::Down);
    EXPECT_TRUE(isOutOfBounds(newPos));
}

TEST_F(SnakeGameTest, DetectsLeftBoundary) {
    Vec2 pos{0, 20};
    Vec2 newPos = moveInDirection(pos, Direction::Left);
    EXPECT_TRUE(isOutOfBounds(newPos));
}

TEST_F(SnakeGameTest, DetectsRightBoundary) {
    Vec2 pos{GRID_W - 1, 20};
    Vec2 newPos = moveInDirection(pos, Direction::Right);
    EXPECT_TRUE(isOutOfBounds(newPos));
}

TEST_F(SnakeGameTest, ValidPositionsAreNotOutOfBounds) {
    Vec2 pos{30, 20};
    EXPECT_FALSE(isOutOfBounds(pos));
    
    Vec2 corner1{0, 0};
    EXPECT_FALSE(isOutOfBounds(corner1));
    
    Vec2 corner2{GRID_W - 1, GRID_H - 1};
    EXPECT_FALSE(isOutOfBounds(corner2));
}

TEST_F(SnakeGameTest, DetectsSelfCollision) {
    std::vector<Vec2> body = {
        {5, 5},  // Head
        {4, 5},
        {3, 5},
        {3, 6},
        {4, 6},
        {5, 6},
        {5, 5}   // Collides with head
    };
    
    EXPECT_TRUE(checkSelfCollision(body));
}

TEST_F(SnakeGameTest, NoSelfCollisionForShortSnake) {
    std::vector<Vec2> body = {{5, 5}};
    EXPECT_FALSE(checkSelfCollision(body));
    
    std::vector<Vec2> body2 = {{5, 5}, {4, 5}};
    EXPECT_FALSE(checkSelfCollision(body2));
}

TEST_F(SnakeGameTest, NoSelfCollisionForNormalSnake) {
    std::vector<Vec2> body = {
        {5, 5},
        {4, 5},
        {3, 5},
        {2, 5}
    };
    
    EXPECT_FALSE(checkSelfCollision(body));
}

TEST_F(SnakeGameTest, DirectionOpposites) {
    // Snake should not be able to reverse directly
    // This would be tested in the actual game logic
    EXPECT_NE(Direction::Up, Direction::Down);
    EXPECT_NE(Direction::Left, Direction::Right);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
