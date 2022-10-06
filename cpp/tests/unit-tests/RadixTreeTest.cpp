/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
 * %%
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * #L%
 */
#include <numeric>
#include <string>
#include <unordered_map>

#include "tests/utils/Gtest.h"

#include "libjoynrclustercontroller/access-control/RadixTree.h"

class RadixTreeTest : public ::testing::Test
{
public:
    RadixTreeTest()
    {
        /*
         * the tree has the following internal structure:
         *
         *                "" (root)
         *                 |
         *                 01 (i)
         *           ______|______
         *          /             \
         *         2               3
         *                      ___|___
         *                     /       \
         *                    4         7
         *                 ___|___
         *                /       \
         *               5         6
         *
         * '(i)' depicts an internal node
         */
        data["012"] = "2";
        data["013"] = "3";
        data["0134"] = "4";
        data["01345"] = "5";
        data["01346"] = "6";
        data["0137"] = "7";

        for (auto& entry : data) {
            tree.insert(entry.first, entry.second);
        }
    }

protected:
    void validateParents(const std::string& key, const std::vector<std::string>& expectedParents)
    {
        Node* node = tree.longestMatch(key);
        ASSERT_TRUE(node);
        auto parents = node->parents();
        auto expectedIt = expectedParents.begin();
        for (auto parentIt = parents.begin(); parentIt != parents.end(); ++parentIt) {
            ASSERT_NE(expectedIt, expectedParents.end());
            Node* parentNode = *parentIt;
            EXPECT_EQ(*expectedIt, parentNode->getValue());
            ++expectedIt;
        }
    }

    using Tree = joynr::RadixTree<std::string, std::string>;
    using Node = typename Tree::Node;
    Tree tree;
    std::unordered_map<std::string, std::string> data;
};

TEST_F(RadixTreeTest, insertReturnsNode)
{
    const std::string key = "key";
    const std::string value = "value";
    Node* node = tree.insert(key, value);

    EXPECT_EQ(value, node->getValue());
}

TEST_F(RadixTreeTest, insertReturnsNodeWithEmptyKey)
{
    const std::string key = "";
    const std::string value = "value";
    Node* node = tree.insert(key, value);

    EXPECT_EQ(value, node->getValue());
}

TEST_F(RadixTreeTest, longestMatchRetrievesCorrectValues)
{
    // find the entries by their original key
    for (auto& entry : data) {
        Node* node = tree.longestMatch(entry.first);
        ASSERT_TRUE(node) << "queried value (entry.first): " << entry.first;
        EXPECT_EQ(entry.second, node->getValue());
    }

    // add some more characters to each key
    // then retrieve the corresponding entry
    for (auto& entry : data) {
        Node* node = tree.longestMatch(entry.first + "####");
        ASSERT_TRUE(node);
        EXPECT_EQ(entry.second, node->getValue());
    }
}

TEST_F(RadixTreeTest, longestMatchDoesNotFindEntryForNonExistingKey)
{
    Node* node = tree.longestMatch("non-existing-key");
    EXPECT_FALSE(node);
}

TEST_F(RadixTreeTest, longestMatchDoesNotFindEntryForInternalNodeKey)
{
    Node* node = tree.longestMatch("01");
    EXPECT_FALSE(node);
}

TEST(RadixTreeTest2, longestMatchDoesNotReturnNonPrefixEntry)
{
    using Tree = joynr::RadixTree<std::string, std::string>;
    using Node = typename Tree::Node;
    Tree longestMatchTestTree;

    const std::string rootKey = "";
    longestMatchTestTree.insert(rootKey, "rootvalue");
    const std::string secondKey = "abc";
    longestMatchTestTree.insert(secondKey, "value1");

    const std::string searchKey = "abxyz";
    Node* resultNode = longestMatchTestTree.longestMatch(searchKey);
    EXPECT_EQ(resultNode->getKey(), rootKey);
}

TEST_F(RadixTreeTest, parentsAreValidWithRootValue)
{
    tree.insert(std::string(""), "root");
    validateParents("01346", {"4", "3", "root"});
    validateParents("01345", {"4", "3", "root"});
    validateParents("0134", {"3", "root"});
    validateParents("0137", {"3", "root"});
    validateParents("013", {"root"});
    validateParents("012", {"root"});
}

TEST_F(RadixTreeTest, parentsAreValidWithoutRootValue)
{
    validateParents("01346", {"4", "3"});
    validateParents("01345", {"4", "3"});
    validateParents("0134", {"3"});
    validateParents("0137", {"3"});
    validateParents("013", {});
    validateParents("012", {});
}

TEST_F(RadixTreeTest, eraseLeaf)
{
    const std::string leafKey = "0137";
    Node* leaf = tree.longestMatch(leafKey);
    ASSERT_TRUE(leaf);
    ASSERT_EQ("7", leaf->getValue());
    leaf->erase();
    Node* node = tree.longestMatch(leafKey);
    ASSERT_EQ("3", node->getValue());
}

TEST_F(RadixTreeTest, eraseMidNodeRestructuresTree)
{
    const std::string midNodeKey = "013";
    Node* midNode = tree.longestMatch(midNodeKey);
    ASSERT_TRUE(midNode);
    midNode->erase();
    Node* node = tree.longestMatch(midNodeKey);
    EXPECT_FALSE(node);

    validateParents("01346", {"4"});
    validateParents("01345", {"4"});
    validateParents("0134", {});
    validateParents("0137", {});
}

TEST_F(RadixTreeTest, eraseRoot)
{
    const std::string rootKey = "";
    tree.insert(rootKey, "root");

    Node* rootNode = tree.longestMatch(rootKey);
    ASSERT_TRUE(rootNode);
    rootNode->erase();
    Node* node = tree.longestMatch(rootKey);
    EXPECT_FALSE(node);

    validateParents("01346", {"4", "3"});
    validateParents("01345", {"4", "3"});
    validateParents("0134", {"3"});
    validateParents("0137", {"3"});
    validateParents("013", {});
    validateParents("012", {});
}

TEST_F(RadixTreeTest, callParentsOnRoot)
{
    const std::string rootKey = "";
    tree.insert(rootKey, "root");
    Node* rootNode = tree.longestMatch(rootKey);
    ASSERT_TRUE(rootNode);
    auto parents = rootNode->parents();
    EXPECT_EQ(parents.begin(), parents.end());
}

TEST_F(RadixTreeTest, visit)
{
    std::size_t nodeCount = 0;
    auto fun = [&nodeCount, data = data](const auto& node, const auto& keyVector) {
        const std::string key =
                std::accumulate(keyVector.begin(),
                                keyVector.end(),
                                std::string(),
                                [](std::string& s, const auto& i) { return s + i.get(); });
        ASSERT_EQ(data.at(key), node.getValue());
        nodeCount++;
    };
    tree.visit(fun);
    EXPECT_EQ(nodeCount, data.size());
}
