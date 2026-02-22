/**
 * Server Player ID Assignment Logic Tests
 * 
 * Tests player ID calculation logic without actual Connection objects.
 */

#include <gtest/gtest.h>
#include <vector>

// Mimic the player ID assignment logic used by GameServer
class PlayerIdAssigner {
public:
    static std::vector<int> assignIds(int controllerCount, int startingId) {
        std::vector<int> ids;
        for (int i = 0; i < controllerCount; i++) {
            ids.push_back(startingId + i);
        }
        return ids;
    }
    
    static int calculateTotalPlayers(const std::vector<int>& controllerCounts) {
        int total = 0;
        for (int count : controllerCounts) {
            total += count;
        }
        return total;
    }
};

class PlayerIdTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(PlayerIdTest, AssignsSequentialIds) {
    auto ids = PlayerIdAssigner::assignIds(2, 0);
    
    ASSERT_EQ(ids.size(), 2);
    EXPECT_EQ(ids[0], 0);
    EXPECT_EQ(ids[1], 1);
}

TEST_F(PlayerIdTest, AssignsIdsWithOffsetStart) {
    auto ids = PlayerIdAssigner::assignIds(2, 5);
    
    ASSERT_EQ(ids.size(), 2);
    EXPECT_EQ(ids[0], 5);
    EXPECT_EQ(ids[1], 6);
}

TEST_F(PlayerIdTest, HandlesZeroControllers) {
    auto ids = PlayerIdAssigner::assignIds(0, 0);
    EXPECT_EQ(ids.size(), 0);
}

TEST_F(PlayerIdTest, HandlesMaxControllers) {
    auto ids = PlayerIdAssigner::assignIds(4, 0);
    
    ASSERT_EQ(ids.size(), 4);
    EXPECT_EQ(ids[0], 0);
    EXPECT_EQ(ids[1], 1);
    EXPECT_EQ(ids[2], 2);
    EXPECT_EQ(ids[3], 3);
}

TEST_F(PlayerIdTest, CalculatesTotalFromMultipleConnections) {
    std::vector<int> counts = {2, 2, 1};  // 3 connections
    int total = PlayerIdAssigner::calculateTotalPlayers(counts);
    EXPECT_EQ(total, 5);
}

TEST_F(PlayerIdTest, CalculateTotalWithNoConnections) {
    std::vector<int> counts;
    int total = PlayerIdAssigner::calculateTotalPlayers(counts);
    EXPECT_EQ(total, 0);
}

TEST_F(PlayerIdTest, MultiConnectionSequentialAssignment) {
    // Simulate 3 connections with different controller counts
    auto conn1Ids = PlayerIdAssigner::assignIds(2, 0);
    auto conn2Ids = PlayerIdAssigner::assignIds(3, 2);
    auto conn3Ids = PlayerIdAssigner::assignIds(1, 5);
    
    // Connection 1: IDs 0-1
    EXPECT_EQ(conn1Ids[0], 0);
    EXPECT_EQ(conn1Ids[1], 1);
    
    // Connection 2: IDs 2-4
    EXPECT_EQ(conn2Ids[0], 2);
    EXPECT_EQ(conn2Ids[1], 3);
    EXPECT_EQ(conn2Ids[2], 4);
    
    // Connection 3: ID 5
    EXPECT_EQ(conn3Ids[0], 5);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
