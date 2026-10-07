#include "TestCommon.h"
#include <Helpers/SequencedKeyList.h>
#include <vector>
#include <string>

TEST(SequencedKeyListTest, PreservesInsertionOrder)
{
    SequencedKeyList list;
    EXPECT_TRUE(list.Empty());
    EXPECT_EQ(list.size(), 0u);

    list.Add("Alpha");
    list.Add("Beta");
    list.Add("Gamma");

    EXPECT_FALSE(list.Empty());
    EXPECT_EQ(list.size(), 3u);

    std::vector<std::string> expected = { "Alpha", "Beta", "Gamma" };
    std::vector<std::string> actual;
    for (const auto& key : list)
    {
        actual.push_back(key.GetString());
    }

    EXPECT_EQ(actual, expected);
}

TEST(SequencedKeyListTest, IgnoresDuplicateAdditions)
{
    SequencedKeyList list;
    list.Add("One");
    list.Add("Two");
    list.Add("One"); // duplicate

    EXPECT_EQ(list.size(), 2u);

    std::vector<std::string> expected = { "One", "Two" };
    std::vector<std::string> actual;
    for (const auto& key : list)
    {
        actual.push_back(key.GetString());
    }

    EXPECT_EQ(actual, expected);
}

TEST(SequencedKeyListTest, ReAddAfterRemovePushedToTailQueueSemantics)
{
    // Verification of FIFO queue semantics:
    // If an item is deleted and added back later, it must be queued at the tail!
    SequencedKeyList list;
    list.Add("Item1");
    list.Add("Item2");
    list.Add("Item3");

    // Remove middle item
    list.Remove("Item2");
    EXPECT_EQ(list.size(), 2u);
    EXPECT_FALSE(list.Contains("Item2"));

    // Re-add Item2
    list.Add("Item2");
    EXPECT_EQ(list.size(), 3u);
    EXPECT_TRUE(list.Contains("Item2"));

    // Sequence MUST now be: Item1, Item3, Item2
    std::vector<std::string> expected = { "Item1", "Item3", "Item2" };
    std::vector<std::string> actual;
    for (const auto& key : list)
    {
        actual.push_back(key.GetString());
    }

    EXPECT_EQ(actual, expected);
}

TEST(SequencedKeyListTest, RemoveHeadAndTail)
{
    SequencedKeyList list;
    list.Add("Head");
    list.Add("Mid");
    list.Add("Tail");

    // Remove Head
    list.Remove("Head");
    EXPECT_FALSE(list.Contains("Head"));
    EXPECT_EQ(list.size(), 2u);

    // Remove Tail
    list.Remove("Tail");
    EXPECT_FALSE(list.Contains("Tail"));
    EXPECT_EQ(list.size(), 1u);

    EXPECT_STREQ((*list.begin()).GetString(), "Mid");
}

TEST(SequencedKeyListTest, ClearResetsState)
{
    SequencedKeyList list;
    for (int i = 0; i < 50; ++i)
    {
        char buf[32];
        sprintf_s(buf, "Key_%d", i);
        list.Add(buf);
    }
    EXPECT_EQ(list.size(), 50u);

    list.Clear();
    EXPECT_TRUE(list.Empty());
    EXPECT_EQ(list.size(), 0u);
    EXPECT_FALSE(list.Contains("Key_0"));
}
