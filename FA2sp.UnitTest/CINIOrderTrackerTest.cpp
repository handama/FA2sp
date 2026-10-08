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

static std::vector<std::string> ToStringVector(const std::vector<ppmfc::CString>& vec)
{
    std::vector<std::string> res;
    res.reserve(vec.size());
    for (const auto& s : vec)
    {
        res.push_back(s.GetString());
    }
    return res;
}

TEST(CINIOrderTrackerTest, RealCINICrudKeyOperations)
{
    CINI ini;
    CINIOrderTracker::Clear(&ini);

    // 1. Initial Insertions and Recording (Simulate load phase)
    ini.WriteString("Basic", "Name", "Island Wars");
    CINIOrderTracker::RecordKey(&ini, "Basic", "Name");

    ini.WriteString("Basic", "Player", "Soviet");
    CINIOrderTracker::RecordKey(&ini, "Basic", "Player");

    ini.WriteString("Basic", "Theme", "NoTheme");
    CINIOrderTracker::RecordKey(&ini, "Basic", "Theme");

    // Query initial state
    EXPECT_TRUE(ini.KeyExists("Basic", "Name"));
    EXPECT_TRUE(ini.KeyExists("Basic", "Player"));
    EXPECT_TRUE(ini.KeyExists("Basic", "Theme"));
    EXPECT_STREQ(ini.GetString("Basic", "Name"), "Island Wars");
    EXPECT_STREQ(ini.GetString("Basic", "Player"), "Soviet");
    EXPECT_STREQ(ini.GetString("Basic", "Theme"), "NoTheme");

    std::vector<std::string> expectedKeys = { "Name", "Player", "Theme" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, "Basic")), expectedKeys);

    // 2. Modify / Update existing key in-place
    // Updating existing key value must NOT alter its sequence order in SequencedKeyList
    ini.WriteString("Basic", "Name", "Island Wars Remastered");
    CINIOrderTracker::RecordKey(&ini, "Basic", "Name");

    EXPECT_STREQ(ini.GetString("Basic", "Name"), "Island Wars Remastered");
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, "Basic")), expectedKeys);

    // 3. Add new key
    // Newly added key AltPalette must be appended to the tail
    ini.WriteString("Basic", "AltPalette", "1");
    CINIOrderTracker::RecordKey(&ini, "Basic", "AltPalette");

    EXPECT_STREQ(ini.GetString("Basic", "AltPalette"), "1");
    expectedKeys = { "Name", "Player", "Theme", "AltPalette" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, "Basic")), expectedKeys);

    // 4. Delete key
    // Remove middle key 'Player'
    bool deleted = ini.DeleteKey("Basic", "Player");
    EXPECT_TRUE(deleted);
    CINIOrderTracker::RemoveKey(&ini, "Basic", "Player");

    EXPECT_FALSE(ini.KeyExists("Basic", "Player"));
    expectedKeys = { "Name", "Theme", "AltPalette" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, "Basic")), expectedKeys);

    // 5. Re-add deleted key -> must be pushed to tail of queue (FIFO queue semantics)
    // Re-write Player="Allies" -> must now be placed at the end
    ini.WriteString("Basic", "Player", "Allies");
    CINIOrderTracker::RecordKey(&ini, "Basic", "Player");

    EXPECT_STREQ(ini.GetString("Basic", "Player"), "Allies");
    expectedKeys = { "Name", "Theme", "AltPalette", "Player" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, "Basic")), expectedKeys);

    CINIOrderTracker::Clear(&ini);
}

TEST(CINIOrderTrackerTest, RealCINICrudSectionOperations)
{
    CINI ini;
    CINIOrderTracker::Clear(&ini);

    // 1. Initial Insertions and Recording across multiple sections
    ini.WriteString("Header", "Version", "4");
    CINIOrderTracker::RecordSection(&ini, "Header");
    CINIOrderTracker::RecordKey(&ini, "Header", "Version");

    ini.WriteString("Map", "Width", "50");
    ini.WriteString("Map", "Height", "50");
    CINIOrderTracker::RecordSection(&ini, "Map");
    CINIOrderTracker::RecordKey(&ini, "Map", "Width");
    CINIOrderTracker::RecordKey(&ini, "Map", "Height");

    ini.WriteString("Lighting", "Ambient", "1.0");
    CINIOrderTracker::RecordSection(&ini, "Lighting");
    CINIOrderTracker::RecordKey(&ini, "Lighting", "Ambient");

    ini.WriteString("Units", "0", "E1,Soviet,256,10,10");
    CINIOrderTracker::RecordSection(&ini, "Units");
    CINIOrderTracker::RecordKey(&ini, "Units", "0");

    // Query initial sections
    std::vector<std::string> expectedSections = { "Header", "Map", "Lighting", "Units" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetSectionNames(&ini)), expectedSections);

    // 2. Modify within existing section (Section order must not be affected)
    ini.WriteString("Map", "Width", "100");
    EXPECT_STREQ(ini.GetString("Map", "Width"), "100");
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetSectionNames(&ini)), expectedSections);

    // 3. Add new section -> must be appended to tail
    ini.WriteString("Triggers", "0", "TrigAlpha");
    CINIOrderTracker::RecordSection(&ini, "Triggers");
    CINIOrderTracker::RecordKey(&ini, "Triggers", "0");

    expectedSections = { "Header", "Map", "Lighting", "Units", "Triggers" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetSectionNames(&ini)), expectedSections);

    // 4. Delete section
    bool deleted = ini.DeleteSection("Lighting");
    EXPECT_TRUE(deleted);
    CINIOrderTracker::RemoveSection(&ini, "Lighting");

    EXPECT_FALSE(ini.SectionExists("Lighting"));
    expectedSections = { "Header", "Map", "Units", "Triggers" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetSectionNames(&ini)), expectedSections);

    // 5. Re-add deleted section -> must be pushed to tail of queue!
    ini.WriteString("Lighting", "Ambient", "1.5");
    CINIOrderTracker::RecordSection(&ini, "Lighting");
    CINIOrderTracker::RecordKey(&ini, "Lighting", "Ambient");

    EXPECT_TRUE(ini.SectionExists("Lighting"));
    EXPECT_STREQ(ini.GetString("Lighting", "Ambient"), "1.5");
    expectedSections = { "Header", "Map", "Units", "Triggers", "Lighting" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetSectionNames(&ini)), expectedSections);

    CINIOrderTracker::Clear(&ini);
}

TEST(CINIOrderTrackerTest, RealCINIAutoTrackingUntrackedItems)
{
    CINI ini;
    CINIOrderTracker::Clear(&ini);

    // 1. Record initial section and keys
    ini.WriteString("Alpha", "k1", "v1");
    ini.WriteString("Alpha", "k2", "v2");
    CINIOrderTracker::RecordSection(&ini, "Alpha");
    CINIOrderTracker::RecordKey(&ini, "Alpha", "k1");
    CINIOrderTracker::RecordKey(&ini, "Alpha", "k2");

    ini.WriteString("Beta", "b1", "vb1");
    CINIOrderTracker::RecordSection(&ini, "Beta");
    CINIOrderTracker::RecordKey(&ini, "Beta", "b1");

    // 2. Add untracked key to 'Alpha' without calling RecordKey
    ini.WriteString("Alpha", "untracked_k3", "v3");

    // 3. Add untracked section 'Gamma' without calling RecordSection
    ini.WriteString("Gamma", "g1", "vg1");

    // Pass 2 of GetEntries should discover untracked_k3, append it and register it
    std::vector<std::string> expectedKeys = { "k1", "k2", "untracked_k3" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, "Alpha", nullptr, true)), expectedKeys);

    // Verify it is now registered in the tracker's internal order
    const auto* kOrder = CINIOrderTracker::GetKeyOrder(&ini, "Alpha");
    ASSERT_NE(kOrder, nullptr);
    EXPECT_EQ(kOrder->size(), 3u);

    // Pass 2 of GetSections should discover Gamma, append it and register it
    std::vector<std::string> expectedSections = { "Alpha", "Beta", "Gamma" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetSectionNames(&ini, true)), expectedSections);

    const auto* sOrder = CINIOrderTracker::GetSectionOrder(&ini);
    ASSERT_NE(sOrder, nullptr);
    EXPECT_EQ(sOrder->size(), 3u);

    CINIOrderTracker::Clear(&ini);
}

TEST(CINIOrderTrackerTest, RealCINIDeletedItemsOmittedDuringIterationEvenWithoutUntrack)
{
    CINI ini;
    CINIOrderTracker::Clear(&ini);

    ini.WriteString("Basic", "Name", "MapName");
    ini.WriteString("Basic", "GhostKey", "WillBeDeleted");
    ini.WriteString("Basic", "Theme", "ThemeA");
    CINIOrderTracker::RecordSection(&ini, "Basic");
    CINIOrderTracker::RecordKey(&ini, "Basic", "Name");
    CINIOrderTracker::RecordKey(&ini, "Basic", "GhostKey");
    CINIOrderTracker::RecordKey(&ini, "Basic", "Theme");

    ini.WriteString("GhostSection", "dummy", "1");
    CINIOrderTracker::RecordSection(&ini, "GhostSection");
    CINIOrderTracker::RecordKey(&ini, "GhostSection", "dummy");

    // Delete key in CINI, but deliberately do NOT call CINIOrderTracker::RemoveKey
    ini.DeleteKey("Basic", "GhostKey");

    // Delete section in CINI, but deliberately do NOT call CINIOrderTracker::RemoveSection
    ini.DeleteSection("GhostSection");

    // GetEntries should skip GhostKey because it no longer exists in CINI
    std::vector<std::string> expectedKeys = { "Name", "Theme" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, "Basic", nullptr, true)), expectedKeys);

    // GetSections should skip GhostSection because it no longer exists in CINI
    std::vector<std::string> expectedSections = { "Basic" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetSectionNames(&ini, true)), expectedSections);

    CINIOrderTracker::Clear(&ini);
}

TEST(CINIOrderTrackerTest, RealCINIPreserveOrderTrueVsNaturalMapOrder)
{
    CINI ini;
    CINIOrderTracker::Clear(&ini);

    // Insert keys where insertion sequence deliberately differs from INISectionEntriesComparator
    // INISectionEntriesComparator sorts: length ascending, then strcmp.
    // Insertion order: "Zebra" (len 5), "Beta" (len 4), "Alpha" (len 5), "A" (len 1)
    // Natural comparator order would be: "A" (len 1), "Beta" (len 4), "Alpha" (len 5), "Zebra" (len 5)
    ini.WriteString("TestSec", "Zebra", "1");
    CINIOrderTracker::RecordKey(&ini, "TestSec", "Zebra");

    ini.WriteString("TestSec", "Beta", "2");
    CINIOrderTracker::RecordKey(&ini, "TestSec", "Beta");

    ini.WriteString("TestSec", "Alpha", "3");
    CINIOrderTracker::RecordKey(&ini, "TestSec", "Alpha");

    ini.WriteString("TestSec", "A", "4");
    CINIOrderTracker::RecordKey(&ini, "TestSec", "A");

    // preserveOrder = true: preserves exact insertion order
    std::vector<std::string> preservedOrder = { "Zebra", "Beta", "Alpha", "A" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, "TestSec", nullptr, true)), preservedOrder);

    // preserveOrder = false: returns natural map dictionary order
    std::vector<std::string> naturalOrder = { "A", "Beta", "Alpha", "Zebra" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, "TestSec", nullptr, false)), naturalOrder);

    CINIOrderTracker::Clear(&ini);
}

TEST(CINIOrderTrackerTest, RealCINIComplexMapLifecycleSimulation)
{
    CINI ini;
    CINIOrderTracker::Clear(&ini);

    // Step 1: Initial map loading simulation
    ini.WriteString("Basic", "Name", "Battleground");
    ini.WriteString("Basic", "Author", "Tester");
    ini.WriteString("Basic", "Player", "Soviet");
    ini.WriteString("Basic", "Theme", "NoTheme");
    CINIOrderTracker::RecordSection(&ini, "Basic");
    CINIOrderTracker::RecordKey(&ini, "Basic", "Name");
    CINIOrderTracker::RecordKey(&ini, "Basic", "Author");
    CINIOrderTracker::RecordKey(&ini, "Basic", "Player");
    CINIOrderTracker::RecordKey(&ini, "Basic", "Theme");

    ini.WriteString("Map", "Theater", "TEMPERATE");
    ini.WriteString("Map", "Size", "0,0,50,50");
    ini.WriteString("Map", "LocalSize", "2,2,46,46");
    CINIOrderTracker::RecordSection(&ini, "Map");
    CINIOrderTracker::RecordKey(&ini, "Map", "Theater");
    CINIOrderTracker::RecordKey(&ini, "Map", "Size");
    CINIOrderTracker::RecordKey(&ini, "Map", "LocalSize");

    ini.WriteString("Waypoints", "0", "100");
    ini.WriteString("Waypoints", "1", "200");
    ini.WriteString("Waypoints", "2", "300");
    ini.WriteString("Waypoints", "3", "400");
    CINIOrderTracker::RecordSection(&ini, "Waypoints");
    CINIOrderTracker::RecordKey(&ini, "Waypoints", "0");
    CINIOrderTracker::RecordKey(&ini, "Waypoints", "1");
    CINIOrderTracker::RecordKey(&ini, "Waypoints", "2");
    CINIOrderTracker::RecordKey(&ini, "Waypoints", "3");

    ini.WriteString("Units", "0", "E1,Soviet,256,10,10");
    ini.WriteString("Units", "1", "HTNK,Soviet,256,12,12");
    CINIOrderTracker::RecordSection(&ini, "Units");
    CINIOrderTracker::RecordKey(&ini, "Units", "0");
    CINIOrderTracker::RecordKey(&ini, "Units", "1");

    // Step 2 (Modify): Rename map and change theater
    ini.WriteString("Basic", "Name", "Battleground 2.0");
    ini.WriteString("Map", "Theater", "SNOW");

    // Step 3 (Delete): Delete waypoint 1
    ini.DeleteKey("Waypoints", "1");
    CINIOrderTracker::RemoveKey(&ini, "Waypoints", "1");

    // Step 4 (Add): Add waypoint 4
    ini.WriteString("Waypoints", "4", "500");
    CINIOrderTracker::RecordKey(&ini, "Waypoints", "4");

    // Step 5 (Delete + Re-add): Move waypoint 0 to tail by deleting and re-adding
    ini.DeleteKey("Waypoints", "0");
    CINIOrderTracker::RemoveKey(&ini, "Waypoints", "0");
    ini.WriteString("Waypoints", "0", "150");
    CINIOrderTracker::RecordKey(&ini, "Waypoints", "0");

    // Step 6 (Add Section): Add new [Triggers] section
    ini.WriteString("Triggers", "0", "TriggerA");
    CINIOrderTracker::RecordSection(&ini, "Triggers");
    CINIOrderTracker::RecordKey(&ini, "Triggers", "0");

    // Step 7 (Delete Section): Delete [Units] section
    ini.DeleteSection("Units");
    CINIOrderTracker::RemoveSection(&ini, "Units");

    // Step 8 (Verify complete final state):
    // Sections order: Basic, Map, Waypoints, Triggers
    std::vector<std::string> expectedSections = { "Basic", "Map", "Waypoints", "Triggers" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetSectionNames(&ini, true)), expectedSections);

    // Basic keys: Name, Author, Player, Theme (values updated, order intact)
    std::vector<std::string> expectedBasicKeys = { "Name", "Author", "Player", "Theme" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, "Basic", nullptr, true)), expectedBasicKeys);
    EXPECT_STREQ(ini.GetString("Basic", "Name"), "Battleground 2.0");
    EXPECT_STREQ(ini.GetString("Basic", "Author"), "Tester");
    EXPECT_STREQ(ini.GetString("Basic", "Player"), "Soviet");
    EXPECT_STREQ(ini.GetString("Basic", "Theme"), "NoTheme");

    // Map keys: Theater, Size, LocalSize
    std::vector<std::string> expectedMapKeys = { "Theater", "Size", "LocalSize" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, "Map", nullptr, true)), expectedMapKeys);
    EXPECT_STREQ(ini.GetString("Map", "Theater"), "SNOW");

    // Waypoints keys: 2, 3, 4, 0 (strictly reflects 1 was deleted, 4 was added, 0 was re-added to tail)
    std::vector<std::string> expectedWaypointKeys = { "2", "3", "4", "0" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, "Waypoints", nullptr, true)), expectedWaypointKeys);
    EXPECT_STREQ(ini.GetString("Waypoints", "2"), "300");
    EXPECT_STREQ(ini.GetString("Waypoints", "3"), "400");
    EXPECT_STREQ(ini.GetString("Waypoints", "4"), "500");
    EXPECT_STREQ(ini.GetString("Waypoints", "0"), "150");

    // Verify GetEntries returns exact key-value pairs in order
    auto waypointEntries = CINIOrderTracker::GetEntries(&ini, "Waypoints", nullptr, true);
    ASSERT_EQ(waypointEntries.size(), 4u);
    EXPECT_STREQ(waypointEntries[0].Key, "2");
    EXPECT_STREQ(waypointEntries[0].Value, "300");
    EXPECT_STREQ(waypointEntries[1].Key, "3");
    EXPECT_STREQ(waypointEntries[1].Value, "400");
    EXPECT_STREQ(waypointEntries[2].Key, "4");
    EXPECT_STREQ(waypointEntries[2].Value, "500");
    EXPECT_STREQ(waypointEntries[3].Key, "0");
    EXPECT_STREQ(waypointEntries[3].Value, "150");

    // Triggers keys: 0
    std::vector<std::string> expectedTriggerKeys = { "0" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, "Triggers", nullptr, true)), expectedTriggerKeys);
    EXPECT_STREQ(ini.GetString("Triggers", "0"), "TriggerA");

    // Units section is gone
    EXPECT_FALSE(ini.SectionExists("Units"));

    CINIOrderTracker::Clear(&ini);
}




