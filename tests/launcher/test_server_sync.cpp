/**
 * Launcher Controller Management Unit Tests
 * 
 * Tests controller count tracking logic.
 * Note: These are behavioral tests without actual network connectivity.
 */

#include <gtest/gtest.h>

class ControllerCountTracker {
    int lastCount = -1;
    
public:
    bool shouldSendUpdate(int newCount) {
        if (newCount == lastCount) {
            return false;  // No change
        }
        lastCount = newCount;
        return true;  // Count changed, should send
    }
    
    int getLastCount() const { return lastCount; }
};

class ControllerManagementTest : public ::testing::Test {
protected:
    ControllerCountTracker tracker;
};

TEST_F(ControllerManagementTest, SendsFirstUpdate) {
    EXPECT_TRUE(tracker.shouldSendUpdate(2));
    EXPECT_EQ(tracker.getLastCount(), 2);
}

TEST_F(ControllerManagementTest, DoesNotSendDuplicateCount) {
    tracker.shouldSendUpdate(2);
    EXPECT_FALSE(tracker.shouldSendUpdate(2));
}

TEST_F(ControllerManagementTest, SendsOnCountChange) {
    tracker.shouldSendUpdate(2);
    EXPECT_TRUE(tracker.shouldSendUpdate(3));
    EXPECT_EQ(tracker.getLastCount(), 3);
}

TEST_F(ControllerManagementTest, HandlesZeroControllers) {
    EXPECT_TRUE(tracker.shouldSendUpdate(0));
    EXPECT_EQ(tracker.getLastCount(), 0);
}

TEST_F(ControllerManagementTest, HandlesMultipleUpdates) {
    EXPECT_TRUE(tracker.shouldSendUpdate(1));
    EXPECT_TRUE(tracker.shouldSendUpdate(2));
    EXPECT_TRUE(tracker.shouldSendUpdate(3));
    EXPECT_FALSE(tracker.shouldSendUpdate(3));
    EXPECT_TRUE(tracker.shouldSendUpdate(2));
    EXPECT_TRUE(tracker.shouldSendUpdate(0));
}

TEST_F(ControllerManagementTest, TracksDecreasingCount) {
    tracker.shouldSendUpdate(4);
    EXPECT_TRUE(tracker.shouldSendUpdate(3));
    EXPECT_TRUE(tracker.shouldSendUpdate(2));
    EXPECT_TRUE(tracker.shouldSendUpdate(1));
    EXPECT_TRUE(tracker.shouldSendUpdate(0));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
