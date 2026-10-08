#include "TestCommon.h"
#include <Helpers/CINIOrderTracker.h>
#include <CINI.h>
#include <Miscs/Hooks.INI.h>
#include <vector>
#include <string>

static void LoadRawINI(CINIExt& ini, const std::string_view raw)
{
    ini.LoadINIExt(reinterpret_cast<uint8_t*>(const_cast<char*>(raw.data())), raw.size(), nullptr, true, true, false);
}

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
    CINIExt ini;
    const char* raw = R"(
[Basic]
Name=Island Wars
Player=Soviet
Theme=NoTheme
)";
    LoadRawINI(ini, raw);

    // Query initial state loaded through LoadINIExt
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
    CINIExt ini;
    const char* raw = R"(
[Header]
Version=4

[Map]
Width=50
Height=50

[Lighting]
Ambient=1.0

[Units]
0=E1,Soviet,256,10,10
)";
    LoadRawINI(ini, raw);

    // Query initial sections loaded through LoadINIExt
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
    CINIExt ini;
    const char* raw = R"(
[Basic]
Name=MapName
GhostKey=WillBeDeleted
Theme=ThemeA

[GhostSection]
dummy=1
)";
    LoadRawINI(ini, raw);

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
    CINIExt ini;
    // Input keys where insertion sequence deliberately differs from INISectionEntriesComparator
    // INISectionEntriesComparator sorts: length ascending, then strcmp.
    // Insertion order: "Zebra" (len 5), "Beta" (len 4), "Alpha" (len 5), "A" (len 1)
    // Natural comparator order would be: "A" (len 1), "Beta" (len 4), "Alpha" (len 5), "Zebra" (len 5)
    const char* raw = R"(
[TestSec]
Zebra=1
Beta=2
Alpha=3
A=4
)";
    LoadRawINI(ini, raw);

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
    CINIExt ini;
    // Step 1: Initial map loading simulation
    const char* raw = R"(
[Basic]
Name=Battleground
Author=Tester
Player=Soviet
Theme=NoTheme

[Map]
Theater=TEMPERATE
Size=0,0,50,50
LocalSize=2,2,46,46

[Waypoints]
0=100
1=200
2=300
3=400

[Units]
0=E1,Soviet,256,10,10
1=HTNK,Soviet,256,12,12
)";
    LoadRawINI(ini, raw);

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

    // Waypoints keys with adaptive sorting: 0, 2, 3, 4 (0 is adaptively sorted to the front instead of trailing at tail)
    std::vector<std::string> expectedWaypointKeys = { "0", "2", "3", "4" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, "Waypoints", nullptr, true)), expectedWaypointKeys);
    EXPECT_STREQ(ini.GetString("Waypoints", "0"), "150");
    EXPECT_STREQ(ini.GetString("Waypoints", "2"), "300");
    EXPECT_STREQ(ini.GetString("Waypoints", "3"), "400");
    EXPECT_STREQ(ini.GetString("Waypoints", "4"), "500");

    // Verify GetEntries returns exact key-value pairs in adaptive order
    auto waypointEntries = CINIOrderTracker::GetEntries(&ini, "Waypoints", nullptr, true);
    ASSERT_EQ(waypointEntries.size(), 4u);
    EXPECT_STREQ(waypointEntries[0].Key, "0");
    EXPECT_STREQ(waypointEntries[0].Value, "150");
    EXPECT_STREQ(waypointEntries[1].Key, "2");
    EXPECT_STREQ(waypointEntries[1].Value, "300");
    EXPECT_STREQ(waypointEntries[2].Key, "3");
    EXPECT_STREQ(waypointEntries[2].Value, "400");
    EXPECT_STREQ(waypointEntries[3].Key, "4");
    EXPECT_STREQ(waypointEntries[3].Value, "500");

    // Triggers keys: 0
    std::vector<std::string> expectedTriggerKeys = { "0" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, "Triggers", nullptr, true)), expectedTriggerKeys);
    EXPECT_STREQ(ini.GetString("Triggers", "0"), "TriggerA");

    // Units section is gone
    EXPECT_FALSE(ini.SectionExists("Units"));

    CINIOrderTracker::Clear(&ini);
}

TEST(CINIOrderTrackerTest, RealCINISameKeyDeletedAndReaddedHeadMiddleTailCombinations)
{
    CINIExt ini;
    const char* sec = "KeyPermutations";
    const char* raw = R"(
[KeyPermutations]
K_Head=1
K_Mid1=2
K_Mid2=3
K_Mid3=4
K_Tail=5
)";
    LoadRawINI(ini, raw);

    std::vector<std::string> expected = { "K_Head", "K_Mid1", "K_Mid2", "K_Mid3", "K_Tail" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, sec)), expected);

    // --- Scenario A: Delete HEAD key (K_Head), verify, then re-add ---
    EXPECT_TRUE(ini.DeleteKey(sec, "K_Head"));
    CINIOrderTracker::RemoveKey(&ini, sec, "K_Head");
    EXPECT_FALSE(ini.KeyExists(sec, "K_Head"));
    EXPECT_STREQ(ini.GetString(sec, "K_Head"), "");
    expected = { "K_Mid1", "K_Mid2", "K_Mid3", "K_Tail" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, sec)), expected);

    // Re-add K_Head -> must be placed at tail
    ini.WriteString(sec, "K_Head", "1_new");
    CINIOrderTracker::RecordKey(&ini, sec, "K_Head");
    EXPECT_TRUE(ini.KeyExists(sec, "K_Head"));
    EXPECT_STREQ(ini.GetString(sec, "K_Head"), "1_new");
    expected = { "K_Mid1", "K_Mid2", "K_Mid3", "K_Tail", "K_Head" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, sec)), expected);

    // --- Scenario B: Delete MIDDLE key (K_Mid2), verify, then re-add ---
    EXPECT_TRUE(ini.DeleteKey(sec, "K_Mid2"));
    CINIOrderTracker::RemoveKey(&ini, sec, "K_Mid2");
    EXPECT_FALSE(ini.KeyExists(sec, "K_Mid2"));
    expected = { "K_Mid1", "K_Mid3", "K_Tail", "K_Head" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, sec)), expected);

    // Re-add K_Mid2 -> must be placed at tail
    ini.WriteString(sec, "K_Mid2", "3_new");
    CINIOrderTracker::RecordKey(&ini, sec, "K_Mid2");
    EXPECT_STREQ(ini.GetString(sec, "K_Mid2"), "3_new");
    expected = { "K_Mid1", "K_Mid3", "K_Tail", "K_Head", "K_Mid2" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, sec)), expected);

    // --- Scenario C: Delete TAIL key (currently K_Mid2), verify, then re-add ---
    EXPECT_TRUE(ini.DeleteKey(sec, "K_Mid2"));
    CINIOrderTracker::RemoveKey(&ini, sec, "K_Mid2");
    EXPECT_FALSE(ini.KeyExists(sec, "K_Mid2"));
    expected = { "K_Mid1", "K_Mid3", "K_Tail", "K_Head" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, sec)), expected);

    // Re-add K_Mid2 again (multi-cycle on the same key!)
    ini.WriteString(sec, "K_Mid2", "3_v3");
    CINIOrderTracker::RecordKey(&ini, sec, "K_Mid2");
    EXPECT_STREQ(ini.GetString(sec, "K_Mid2"), "3_v3");
    expected = { "K_Mid1", "K_Mid3", "K_Tail", "K_Head", "K_Mid2" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, sec)), expected);

    // --- Scenario D: Permutation delete Head, Mid, Tail, then re-add in reverse order ---
    // Current order: ["K_Mid1" (Head), "K_Mid3" (Mid), "K_Tail" (Mid), "K_Head" (Mid), "K_Mid2" (Tail)]
    // Delete Head ("K_Mid1")
    ini.DeleteKey(sec, "K_Mid1");
    CINIOrderTracker::RemoveKey(&ini, sec, "K_Mid1");
    // Delete Mid ("K_Tail")
    ini.DeleteKey(sec, "K_Tail");
    CINIOrderTracker::RemoveKey(&ini, sec, "K_Tail");
    // Delete Tail ("K_Mid2")
    ini.DeleteKey(sec, "K_Mid2");
    CINIOrderTracker::RemoveKey(&ini, sec, "K_Mid2");

    // Remaining should be: ["K_Mid3", "K_Head"]
    expected = { "K_Mid3", "K_Head" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, sec)), expected);

    // Re-add in reverse order: K_Mid2 (former tail), K_Tail (former mid), K_Mid1 (former head)
    ini.WriteString(sec, "K_Mid2", "200");
    CINIOrderTracker::RecordKey(&ini, sec, "K_Mid2");
    ini.WriteString(sec, "K_Tail", "300");
    CINIOrderTracker::RecordKey(&ini, sec, "K_Tail");
    ini.WriteString(sec, "K_Mid1", "100");
    CINIOrderTracker::RecordKey(&ini, sec, "K_Mid1");

    expected = { "K_Mid3", "K_Head", "K_Mid2", "K_Tail", "K_Mid1" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, sec)), expected);

    // Verify all values are retrievable correctly
    EXPECT_STREQ(ini.GetString(sec, "K_Mid3"), "4");
    EXPECT_STREQ(ini.GetString(sec, "K_Head"), "1_new");
    EXPECT_STREQ(ini.GetString(sec, "K_Mid2"), "200");
    EXPECT_STREQ(ini.GetString(sec, "K_Tail"), "300");
    EXPECT_STREQ(ini.GetString(sec, "K_Mid1"), "100");

    // --- Scenario E: Delete all remaining keys to empty, then re-populate ---
    for (const auto& k : expected)
    {
        EXPECT_TRUE(ini.DeleteKey(sec, k.c_str()));
        CINIOrderTracker::RemoveKey(&ini, sec, k.c_str());
    }
    EXPECT_EQ(ini.GetKeyCount(sec), 0);
    EXPECT_TRUE(CINIOrderTracker::GetKeyNames(&ini, sec).empty());

    // Re-add in a new order
    ini.WriteString(sec, "Reborn_A", "A");
    ini.WriteString(sec, "Reborn_B", "B");
    CINIOrderTracker::RecordKey(&ini, sec, "Reborn_A");
    CINIOrderTracker::RecordKey(&ini, sec, "Reborn_B");
    expected = { "Reborn_A", "Reborn_B" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, sec)), expected);

    CINIOrderTracker::Clear(&ini);
}

TEST(CINIOrderTrackerTest, RealCINISameSectionDeletedAndReaddedHeadMiddleTailCombinations)
{
    CINIExt ini;
    const char* raw = R"(
[Sec_Head]
val=0

[Sec_Mid1]
val=1

[Sec_Mid2]
val=2

[Sec_Mid3]
val=3

[Sec_Tail]
val=4
)";
    LoadRawINI(ini, raw);
    const std::vector<std::string> initSections = { "Sec_Head", "Sec_Mid1", "Sec_Mid2", "Sec_Mid3", "Sec_Tail" };

    std::vector<std::string> expected = initSections;
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetSectionNames(&ini)), expected);

    // --- Scenario A: Delete HEAD section ("Sec_Head"), verify, then re-add ---
    EXPECT_TRUE(ini.DeleteSection("Sec_Head"));
    CINIOrderTracker::RemoveSection(&ini, "Sec_Head");
    EXPECT_FALSE(ini.SectionExists("Sec_Head"));
    expected = { "Sec_Mid1", "Sec_Mid2", "Sec_Mid3", "Sec_Tail" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetSectionNames(&ini)), expected);

    // Re-add "Sec_Head" -> must move to tail
    ini.WriteString("Sec_Head", "new_val", "100");
    CINIOrderTracker::RecordSection(&ini, "Sec_Head");
    CINIOrderTracker::RecordKey(&ini, "Sec_Head", "new_val");
    EXPECT_TRUE(ini.SectionExists("Sec_Head"));
    EXPECT_STREQ(ini.GetString("Sec_Head", "new_val"), "100");
    expected = { "Sec_Mid1", "Sec_Mid2", "Sec_Mid3", "Sec_Tail", "Sec_Head" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetSectionNames(&ini)), expected);

    // --- Scenario B: Delete MIDDLE section ("Sec_Mid2"), verify, then re-add ---
    EXPECT_TRUE(ini.DeleteSection("Sec_Mid2"));
    CINIOrderTracker::RemoveSection(&ini, "Sec_Mid2");
    EXPECT_FALSE(ini.SectionExists("Sec_Mid2"));
    expected = { "Sec_Mid1", "Sec_Mid3", "Sec_Tail", "Sec_Head" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetSectionNames(&ini)), expected);

    // Re-add "Sec_Mid2" -> must move to tail
    ini.WriteString("Sec_Mid2", "mid_val", "200");
    CINIOrderTracker::RecordSection(&ini, "Sec_Mid2");
    CINIOrderTracker::RecordKey(&ini, "Sec_Mid2", "mid_val");
    expected = { "Sec_Mid1", "Sec_Mid3", "Sec_Tail", "Sec_Head", "Sec_Mid2" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetSectionNames(&ini)), expected);

    // --- Scenario C: Delete TAIL section (currently "Sec_Mid2"), verify, then re-add ---
    EXPECT_TRUE(ini.DeleteSection("Sec_Mid2"));
    CINIOrderTracker::RemoveSection(&ini, "Sec_Mid2");
    EXPECT_FALSE(ini.SectionExists("Sec_Mid2"));
    expected = { "Sec_Mid1", "Sec_Mid3", "Sec_Tail", "Sec_Head" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetSectionNames(&ini)), expected);

    // Re-add "Sec_Mid2" again (multi-cycle on the same section!)
    ini.WriteString("Sec_Mid2", "mid_val_v3", "300");
    CINIOrderTracker::RecordSection(&ini, "Sec_Mid2");
    CINIOrderTracker::RecordKey(&ini, "Sec_Mid2", "mid_val_v3");
    expected = { "Sec_Mid1", "Sec_Mid3", "Sec_Tail", "Sec_Head", "Sec_Mid2" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetSectionNames(&ini)), expected);

    // --- Scenario D: Permutation delete Head, Mid, Tail, then re-add in reverse order ---
    // Current sections: ["Sec_Mid1" (Head), "Sec_Mid3" (Mid), "Sec_Tail" (Mid), "Sec_Head" (Mid), "Sec_Mid2" (Tail)]
    // Delete Head ("Sec_Mid1")
    ini.DeleteSection("Sec_Mid1");
    CINIOrderTracker::RemoveSection(&ini, "Sec_Mid1");
    // Delete Mid ("Sec_Tail")
    ini.DeleteSection("Sec_Tail");
    CINIOrderTracker::RemoveSection(&ini, "Sec_Tail");
    // Delete Tail ("Sec_Mid2")
    ini.DeleteSection("Sec_Mid2");
    CINIOrderTracker::RemoveSection(&ini, "Sec_Mid2");

    // Remaining should be: ["Sec_Mid3", "Sec_Head"]
    expected = { "Sec_Mid3", "Sec_Head" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetSectionNames(&ini)), expected);

    // Re-add in reverse order: Sec_Mid2 (former tail), Sec_Tail (former mid), Sec_Mid1 (former head)
    ini.WriteString("Sec_Mid2", "v", "1");
    CINIOrderTracker::RecordSection(&ini, "Sec_Mid2");
    ini.WriteString("Sec_Tail", "v", "2");
    CINIOrderTracker::RecordSection(&ini, "Sec_Tail");
    ini.WriteString("Sec_Mid1", "v", "3");
    CINIOrderTracker::RecordSection(&ini, "Sec_Mid1");

    expected = { "Sec_Mid3", "Sec_Head", "Sec_Mid2", "Sec_Tail", "Sec_Mid1" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetSectionNames(&ini)), expected);

    // --- Scenario E: Delete all sections to empty, then re-populate in reverse ---
    for (const auto& secName : expected)
    {
        EXPECT_TRUE(ini.DeleteSection(secName.c_str()));
        CINIOrderTracker::RemoveSection(&ini, secName.c_str());
    }
    EXPECT_TRUE(CINIOrderTracker::GetSectionNames(&ini).empty());

    // Re-populate in reversed order
    ini.WriteString("Z_Sec", "k", "1");
    CINIOrderTracker::RecordSection(&ini, "Z_Sec");
    ini.WriteString("A_Sec", "k", "2");
    CINIOrderTracker::RecordSection(&ini, "A_Sec");

    expected = { "Z_Sec", "A_Sec" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetSectionNames(&ini)), expected);

    CINIOrderTracker::Clear(&ini);
}

TEST(CINIOrderTrackerTest, RealCINIInterleavedKeyAndSectionDeleteReaddStressTest)
{
    CINIExt ini;
    // Create 3 sections, each with 3 keys: Head, Mid, Tail
    const char* raw = R"(
[Section_1]
Key_H=val_H
Key_M=val_M
Key_T=val_T

[Section_2]
Key_H=val_H
Key_M=val_M
Key_T=val_T

[Section_3]
Key_H=val_H
Key_M=val_M
Key_T=val_T
)";
    LoadRawINI(ini, raw);

    // Step 1: In Section_1 (Head section), delete Head key Key_H and re-add it
    ini.DeleteKey("Section_1", "Key_H");
    CINIOrderTracker::RemoveKey(&ini, "Section_1", "Key_H");
    ini.WriteString("Section_1", "Key_H", "val_H_reborn");
    CINIOrderTracker::RecordKey(&ini, "Section_1", "Key_H");

    std::vector<std::string> expectedKeysSec1 = { "Key_M", "Key_T", "Key_H" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, "Section_1")), expectedKeysSec1);
    EXPECT_STREQ(ini.GetString("Section_1", "Key_H"), "val_H_reborn");

    // Step 2: Delete Section_1 entirely (Head section deleted)
    ini.DeleteSection("Section_1");
    CINIOrderTracker::RemoveSection(&ini, "Section_1");

    std::vector<std::string> expectedSections = { "Section_2", "Section_3" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetSectionNames(&ini)), expectedSections);

    // Step 3: In Section_2 (now Head section), delete Tail key Key_T, then delete Mid key Key_M
    ini.DeleteKey("Section_2", "Key_T");
    CINIOrderTracker::RemoveKey(&ini, "Section_2", "Key_T");
    ini.DeleteKey("Section_2", "Key_M");
    CINIOrderTracker::RemoveKey(&ini, "Section_2", "Key_M");

    std::vector<std::string> expectedKeysSec2 = { "Key_H" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, "Section_2")), expectedKeysSec2);

    // Re-add Key_M then Key_T in Section_2
    ini.WriteString("Section_2", "Key_M", "val_M_new");
    CINIOrderTracker::RecordKey(&ini, "Section_2", "Key_M");
    ini.WriteString("Section_2", "Key_T", "val_T_new");
    CINIOrderTracker::RecordKey(&ini, "Section_2", "Key_T");

    expectedKeysSec2 = { "Key_H", "Key_M", "Key_T" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, "Section_2")), expectedKeysSec2);

    // Step 4: Re-add Section_1 (must now become the TAIL section)
    ini.WriteString("Section_1", "Alpha", "1");
    ini.WriteString("Section_1", "Beta", "2");
    CINIOrderTracker::RecordSection(&ini, "Section_1");
    CINIOrderTracker::RecordKey(&ini, "Section_1", "Alpha");
    CINIOrderTracker::RecordKey(&ini, "Section_1", "Beta");

    expectedSections = { "Section_2", "Section_3", "Section_1" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetSectionNames(&ini)), expectedSections);

    expectedKeysSec1 = { "Alpha", "Beta" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, "Section_1")), expectedKeysSec1);

    // Step 5: Delete Section_3 (Middle section deleted)
    ini.DeleteSection("Section_3");
    CINIOrderTracker::RemoveSection(&ini, "Section_3");

    expectedSections = { "Section_2", "Section_1" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetSectionNames(&ini)), expectedSections);

    // Step 6: Re-add Section_3 (becomes TAIL section)
    ini.WriteString("Section_3", "Gamma", "3");
    CINIOrderTracker::RecordSection(&ini, "Section_3");
    CINIOrderTracker::RecordKey(&ini, "Section_3", "Gamma");

    expectedSections = { "Section_2", "Section_1", "Section_3" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetSectionNames(&ini)), expectedSections);

    // Step 7: Verify all entries across all sections
    auto sec2Entries = CINIOrderTracker::GetEntries(&ini, "Section_2");
    ASSERT_EQ(sec2Entries.size(), 3u);
    EXPECT_STREQ(sec2Entries[0].Key, "Key_H");
    EXPECT_STREQ(sec2Entries[0].Value, "val_H");
    EXPECT_STREQ(sec2Entries[1].Key, "Key_M");
    EXPECT_STREQ(sec2Entries[1].Value, "val_M_new");
    EXPECT_STREQ(sec2Entries[2].Key, "Key_T");
    EXPECT_STREQ(sec2Entries[2].Value, "val_T_new");

    auto sec1Entries = CINIOrderTracker::GetEntries(&ini, "Section_1");
    ASSERT_EQ(sec1Entries.size(), 2u);
    EXPECT_STREQ(sec1Entries[0].Key, "Alpha");
    EXPECT_STREQ(sec1Entries[0].Value, "1");
    EXPECT_STREQ(sec1Entries[1].Key, "Beta");
    EXPECT_STREQ(sec1Entries[1].Value, "2");

    auto sec3Entries = CINIOrderTracker::GetEntries(&ini, "Section_3");
    ASSERT_EQ(sec3Entries.size(), 1u);
    EXPECT_STREQ(sec3Entries[0].Key, "Gamma");
    EXPECT_STREQ(sec3Entries[0].Value, "3");

    CINIOrderTracker::Clear(&ini);
}

TEST(CINIOrderTrackerTest, AdaptiveSortingNumericKeysPreserveNaturalOrderOnDeleteAndReadd)
{
    CINIExt ini;
    const char* raw = R"(
[Units]
0=Unit_Zero
1=Unit_One
2=Unit_Two
3=Unit_Three
)";
    LoadRawINI(ini, raw);

    std::vector<std::string> expectedInitial = { "0", "1", "2", "3" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, "Units", nullptr, true)), expectedInitial);

    // Case 1: Head deletion and re-add of "0"
    // Without adaptive sorting, "0" would end up at the tail: {"1", "2", "3", "0"}
    // With adaptive sorting, "0" must return to the front: {"0", "1", "2", "3"}
    ini.DeleteKey("Units", "0");
    CINIOrderTracker::RemoveKey(&ini, "Units", "0");

    ini.WriteString("Units", "0", "Unit_Zero_Readded");
    CINIOrderTracker::RecordKey(&ini, "Units", "0");

    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, "Units", nullptr, true)), expectedInitial);
    auto entries0 = CINIOrderTracker::GetEntries(&ini, "Units", nullptr, true);
    ASSERT_EQ(entries0.size(), 4u);
    EXPECT_STREQ(entries0[0].Key, "0");
    EXPECT_STREQ(entries0[0].Value, "Unit_Zero_Readded");

    // Case 2: Middle deletion and re-add of "2"
    ini.DeleteKey("Units", "2");
    CINIOrderTracker::RemoveKey(&ini, "Units", "2");

    ini.WriteString("Units", "2", "Unit_Two_Readded");
    CINIOrderTracker::RecordKey(&ini, "Units", "2");

    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, "Units", nullptr, true)), expectedInitial);
    auto entries2 = CINIOrderTracker::GetEntries(&ini, "Units", nullptr, true);
    EXPECT_STREQ(entries2[2].Key, "2");
    EXPECT_STREQ(entries2[2].Value, "Unit_Two_Readded");

    // Case 3: Tail deletion and re-add of "3"
    ini.DeleteKey("Units", "3");
    CINIOrderTracker::RemoveKey(&ini, "Units", "3");

    ini.WriteString("Units", "3", "Unit_Three_Readded");
    CINIOrderTracker::RecordKey(&ini, "Units", "3");

    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, "Units", nullptr, true)), expectedInitial);

    // Case 4: Delete multiple keys ("1" and "0"), re-add in reverse order ("1" then "0")
    ini.DeleteKey("Units", "1");
    CINIOrderTracker::RemoveKey(&ini, "Units", "1");
    ini.DeleteKey("Units", "0");
    CINIOrderTracker::RemoveKey(&ini, "Units", "0");

    ini.WriteString("Units", "1", "Unit_One_V2");
    CINIOrderTracker::RecordKey(&ini, "Units", "1");
    ini.WriteString("Units", "0", "Unit_Zero_V2");
    CINIOrderTracker::RecordKey(&ini, "Units", "0");

    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, "Units", nullptr, true)), expectedInitial);
    auto entriesMulti = CINIOrderTracker::GetEntries(&ini, "Units", nullptr, true);
    EXPECT_STREQ(entriesMulti[0].Key, "0");
    EXPECT_STREQ(entriesMulti[0].Value, "Unit_Zero_V2");
    EXPECT_STREQ(entriesMulti[1].Key, "1");
    EXPECT_STREQ(entriesMulti[1].Value, "Unit_One_V2");

    CINIOrderTracker::Clear(&ini);
}

TEST(CINIOrderTrackerTest, AdaptiveSortingWestwoodNumericComparatorMultiDigit)
{
    CINIExt ini;
    // Multi-digit numbers written in raw INI in intentionally scrambled order
    // "100", "10", "0", "9", "1", "11", "8", "99"
    // In ASCII alphabetical sort: "0", "1", "10", "100", "11", "8", "9", "99" (WRONG for game engine)
    // In Westwood numeric sort:   "0", "1", "8", "9", "10", "11", "99", "100" (CORRECT)
    const char* raw = R"(
[Waypoints]
100=pos_100
10=pos_10
0=pos_0
9=pos_9
1=pos_1
11=pos_11
8=pos_8
99=pos_99
)";
    LoadRawINI(ini, raw);

    std::vector<std::string> expectedWestwoodOrder = { "0", "1", "8", "9", "10", "11", "99", "100" };
    auto actualKeys = ToStringVector(CINIOrderTracker::GetKeyNames(&ini, "Waypoints", nullptr, true));
    EXPECT_EQ(actualKeys, expectedWestwoodOrder);

    CINIOrderTracker::Clear(&ini);
}

TEST(CINIOrderTrackerTest, AdaptiveSortingMixedSectionPreservesTextHeaderAndOrdersNumericArray)
{
    CINIExt ini;
    // Simulate Script / TaskForce section like [01000001]
    // Header properties: Name, Group
    // Numeric action/member entries: 0, 1, 2
    const char* raw = R"(
[01000001]
Name=Assault Squad
Group=-1
0=1,E1
1=2,HTK
2=1,APOC
)";
    LoadRawINI(ini, raw);

    const char* sec = "01000001";
    // Delete action "0", then re-add "0" -> should stay right after headers and before "1"
    ini.DeleteKey(sec, "0");
    CINIOrderTracker::RemoveKey(&ini, sec, "0");

    ini.WriteString(sec, "0", "1,E1_New");
    CINIOrderTracker::RecordKey(&ini, sec, "0");

    std::vector<std::string> expectedOrder = { "Name", "Group", "0", "1", "2" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, sec, nullptr, true)), expectedOrder);

    auto entries = CINIOrderTracker::GetEntries(&ini, sec, nullptr, true);
    ASSERT_EQ(entries.size(), 5u);
    EXPECT_STREQ(entries[0].Key, "Name");
    EXPECT_STREQ(entries[0].Value, "Assault Squad");
    EXPECT_STREQ(entries[1].Key, "Group");
    EXPECT_STREQ(entries[1].Value, "-1");
    EXPECT_STREQ(entries[2].Key, "0");
    EXPECT_STREQ(entries[2].Value, "1,E1_New");
    EXPECT_STREQ(entries[3].Key, "1");
    EXPECT_STREQ(entries[3].Value, "2,HTK");
    EXPECT_STREQ(entries[4].Key, "2");
    EXPECT_STREQ(entries[4].Value, "1,APOC");

    CINIOrderTracker::Clear(&ini);
}

TEST(CINIOrderTrackerTest, AdaptiveSortingUntrackedKeyWithoutManualRecordKeyAutoSorted)
{
    CINIExt ini;
    const char* raw = R"(
[Units]
1=Unit_One
2=Unit_Two
3=Unit_Three
)";
    LoadRawINI(ini, raw);

    // Call WriteString alone without ANY manual RecordKey call
    ini.WriteString("Units", "0", "Unit_Zero_Auto");

    // Pass 2 should auto-discover "0", Pass 3 should adaptively sort it to index 0
    std::vector<std::string> expectedOrder = { "0", "1", "2", "3" };
    EXPECT_EQ(ToStringVector(CINIOrderTracker::GetKeyNames(&ini, "Units", nullptr, true)), expectedOrder);

    auto entries = CINIOrderTracker::GetEntries(&ini, "Units", nullptr, true);
    ASSERT_EQ(entries.size(), 4u);
    EXPECT_STREQ(entries[0].Key, "0");
    EXPECT_STREQ(entries[0].Value, "Unit_Zero_Auto");
    EXPECT_STREQ(entries[1].Key, "1");
    EXPECT_STREQ(entries[1].Value, "Unit_One");

    CINIOrderTracker::Clear(&ini);
}
