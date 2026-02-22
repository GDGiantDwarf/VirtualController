/**
 * Karting Game Logic Unit Tests
 *
 * Tests core mechanics using lightweight stand-ins for game logic.
 * These are mock tests since the actual game logic is embedded in the client.
 */

#include <gtest/gtest.h>
#include <algorithm>

namespace {

constexpr int LAP_GOAL = 3;
constexpr int FINISH_LAPS = LAP_GOAL + 1;
constexpr float MAX_SPEED = 7.5f;
constexpr float ACCELERATION = 0.2f;
constexpr float FRICTION = 0.92f;
constexpr float ROTATION_SPEED = 3.0f;

struct CarState {
    float speed{0.0f};
    float rotation{0.0f};
    int lapsCompleted{0};
    bool finishedRace{false};
};

void applyInputState(CarState& car, float throttle, float steer) {
    float clampedThrottle = std::clamp(throttle, -1.0f, 1.0f);
    float clampedSteer = std::clamp(steer, -1.0f, 1.0f);

    car.speed = std::clamp(
        car.speed + (ACCELERATION * clampedThrottle),
        -MAX_SPEED * 0.5f,
        MAX_SPEED
    );
    car.rotation += ROTATION_SPEED * clampedSteer;
}

void applyFriction(CarState& car) {
    car.speed *= FRICTION;
}

void updateFinishedState(CarState& car) {
    if (car.lapsCompleted >= FINISH_LAPS) {
        car.finishedRace = true;
    }
}

int displayLapCount(int lapsCompleted) {
    return std::max(0, lapsCompleted - 1);
}

} // namespace

TEST(KartingGameTest, AccelerationClampsToMaxSpeed) {
    CarState car;
    for (int i = 0; i < 200; ++i) {
        applyInputState(car, 1.0f, 0.0f);
    }
    EXPECT_FLOAT_EQ(car.speed, MAX_SPEED);
}

TEST(KartingGameTest, ReverseSpeedClampsToHalfMax) {
    CarState car;
    for (int i = 0; i < 200; ++i) {
        applyInputState(car, -1.0f, 0.0f);
    }
    EXPECT_FLOAT_EQ(car.speed, -MAX_SPEED * 0.5f);
}

TEST(KartingGameTest, RotationRespondsToSteer) {
    CarState car;
    applyInputState(car, 0.0f, 1.0f);
    EXPECT_FLOAT_EQ(car.rotation, ROTATION_SPEED);

    applyInputState(car, 0.0f, -1.0f);
    EXPECT_FLOAT_EQ(car.rotation, 0.0f);
}

TEST(KartingGameTest, FrictionReducesSpeed) {
    CarState car;
    car.speed = 10.0f;
    applyFriction(car);
    EXPECT_FLOAT_EQ(car.speed, 10.0f * FRICTION);
}

TEST(KartingGameTest, FinishLapThresholdUsesFinishLaps) {
    CarState car;
    car.lapsCompleted = FINISH_LAPS - 1;
    updateFinishedState(car);
    EXPECT_FALSE(car.finishedRace);

    car.lapsCompleted = FINISH_LAPS;
    updateFinishedState(car);
    EXPECT_TRUE(car.finishedRace);
}

TEST(KartingGameTest, EndScreenLapDisplayIsClamped) {
    EXPECT_EQ(displayLapCount(0), 0);
    EXPECT_EQ(displayLapCount(1), 0);
    EXPECT_EQ(displayLapCount(2), 1);
    EXPECT_EQ(displayLapCount(4), 3);
}
