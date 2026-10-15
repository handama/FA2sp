#include "TestCommon.h"
#include <vector>
#include <algorithm>

TEST(SHPVehicleFacingTest, ZeroOffsetDoesNotRotateFrames)
{
    // When SHPVehicle.FacingOffset = 0 (RN configuration):
    // std::rotate(begin, begin + 0 * targetFacings / 8, end) is a no-op.
    const int offset = 0;
    const int targetFacings = 8;
    std::vector<int> framesToRead = { 0, 1, 2, 3, 4, 5, 6, 7 };

    std::rotate(framesToRead.begin(), framesToRead.begin() + offset * targetFacings / 8, framesToRead.end());

    std::vector<int> expected = { 0, 1, 2, 3, 4, 5, 6, 7 };
    EXPECT_EQ(framesToRead, expected);
    EXPECT_EQ(framesToRead[0], 0);
}

TEST(SHPVehicleFacingTest, ZeroOffsetTurretFacingAlignsWithBody)
{
    // Turret facing formula: (((offset * targetFacings / 8 + i) % targetFacings) * 32 / targetFacings) % 32
    // With offset = 0, facing 0 produces turret facing 0, perfectly aligned with body frame 0.
    const int offset = 0;
    const int targetFacings = 8;

    for (int i = 0; i < targetFacings; ++i)
    {
        int turrentFacing = (((offset * targetFacings / 8 + i) % targetFacings) * 32 / targetFacings) % 32;
        EXPECT_EQ(turrentFacing, i * (32 / targetFacings));
    }

    int facing0Turret = (((offset * targetFacings / 8 + 0) % targetFacings) * 32 / targetFacings) % 32;
    EXPECT_EQ(facing0Turret, 0);
}

TEST(SHPVehicleFacingTest, LegacyOffsetOneShiftsFramesAndTurretBy45Degrees)
{
    // When SHPVehicle.FacingOffset = 1 (legacy FA2sp cmcc patch default):
    // Shifts frames forward by 1 step (45 degrees), reading frame 1 for facing 0.
    const int offset = 1;
    const int targetFacings = 8;
    std::vector<int> framesToRead = { 0, 1, 2, 3, 4, 5, 6, 7 };

    std::rotate(framesToRead.begin(), framesToRead.begin() + offset * targetFacings / 8, framesToRead.end());

    std::vector<int> expected = { 1, 2, 3, 4, 5, 6, 7, 0 };
    EXPECT_EQ(framesToRead, expected);
    EXPECT_EQ(framesToRead[0], 1);

    // Turret facing also shifted by +4 (45 degrees in 32-facing space)
    int facing0Turret = (((offset * targetFacings / 8 + 0) % targetFacings) * 32 / targetFacings) % 32;
    EXPECT_EQ(facing0Turret, 4);
}

TEST(SHPVehicleFacingTest, ExtFacings32Support)
{
    // Verification with 32 facings (ExtFacings enabled):
    const int targetFacings = 32;

    // Offset = 0: no rotate
    {
        const int offset = 0;
        std::vector<int> frames(32);
        for (int i = 0; i < 32; ++i) frames[i] = i;

        std::rotate(frames.begin(), frames.begin() + offset * targetFacings / 8, frames.end());
        EXPECT_EQ(frames[0], 0);

        int turrentFacing = (((offset * targetFacings / 8 + 0) % targetFacings) * 32 / targetFacings) % 32;
        EXPECT_EQ(turrentFacing, 0);
    }

    // Offset = 1: rotate by 4 steps (32 / 8 = 4, 45 degrees)
    {
        const int offset = 1;
        std::vector<int> frames(32);
        for (int i = 0; i < 32; ++i) frames[i] = i;

        std::rotate(frames.begin(), frames.begin() + offset * targetFacings / 8, frames.end());
        EXPECT_EQ(frames[0], 4);

        int turrentFacing = (((offset * targetFacings / 8 + 0) % targetFacings) * 32 / targetFacings) % 32;
        EXPECT_EQ(turrentFacing, 4);
    }
}
