#include "TestCommon.h"
#include <Helpers/CINIOrderTracker.h>
#include <CINI.h>
#include <vector>
#include <string>

TEST(CINIOrderTrackerTest, SectionOrderTrackingAndQueueSemantics)
{
    // Dummy pointer for tracking
    CINI* pDummy = reinterpret_cast<CINI*>(0x12340000);
    CINIOrderTracker::Clear(pDummy);

    CINIOrderTracker::RecordSection(pDummy, "Header");
    CINIOrderTracker::RecordSection(pDummy, "Map");
    CINIOrderTracker::RecordSection(pDummy, "Basic");

    const auto* order = CINIOrderTracker::GetSectionOrder(pDummy);
    ASSERT_NE(order, nullptr);
    EXPECT_EQ(order->size(), 3u);

    // Remove Map, then re-record Map -> must be at the end!
    CINIOrderTracker::RemoveSection(pDummy, "Map");
    EXPECT_EQ(order->size(), 2u);

    CINIOrderTracker::RecordSection(pDummy, "Map");
    EXPECT_EQ(order->size(), 3u);

    std::vector<std::string> expected = { "Header", "Basic", "Map" };
    std::vector<std::string> actual;
    for (const auto& sec : *order)
    {
        actual.push_back(sec.GetString());
    }
    EXPECT_EQ(actual, expected);

    CINIOrderTracker::Clear(pDummy);
    EXPECT_EQ(CINIOrderTracker::GetSectionOrder(pDummy), nullptr);
}

TEST(CINIOrderTrackerTest, KeyOrderTrackingAndQueueSemantics)
{
    CINI* pDummy = reinterpret_cast<CINI*>(0x12340000);
    CINIOrderTracker::Clear(pDummy);

    CINIOrderTracker::RecordKey(pDummy, "Basic", "Name");
    CINIOrderTracker::RecordKey(pDummy, "Basic", "Player");
    CINIOrderTracker::RecordKey(pDummy, "Basic", "Theme");

    const auto* kOrder = CINIOrderTracker::GetKeyOrder(pDummy, "Basic");
    ASSERT_NE(kOrder, nullptr);
    EXPECT_EQ(kOrder->size(), 3u);

    // Remove Player, then re-record Player -> must be moved to the end!
    CINIOrderTracker::RemoveKey(pDummy, "Basic", "Player");
    EXPECT_EQ(kOrder->size(), 2u);

    CINIOrderTracker::RecordKey(pDummy, "Basic", "Player");
    EXPECT_EQ(kOrder->size(), 3u);

    std::vector<std::string> expected = { "Name", "Theme", "Player" };
    std::vector<std::string> actual;
    for (const auto& k : *kOrder)
    {
        actual.push_back(k.GetString());
    }
    EXPECT_EQ(actual, expected);

    CINIOrderTracker::Clear(pDummy);
}
