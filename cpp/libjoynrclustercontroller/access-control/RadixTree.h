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

#ifndef ACCESS_CONTROL_RADIXTREE_H
#define ACCESS_CONTROL_RADIXTREE_H

#include <cassert>

#include <algorithm>
#include <functional>

#include <map>
#include <memory>
#include <utility>
#include <vector>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/optional.hpp>

namespace joynr
{

template <typename Key, typename Value>
class RadixTreeNode
{

private:
    using ChildPtr = std::unique_ptr<RadixTreeNode>;
    using ChildMap = std::map<Key, ChildPtr>;
    template <typename K, typename V>
    friend class RadixTree;

public:
    template <typename K, typename V>
    class ParentIterator : public boost::iterator_facade<ParentIterator<K, V>,
                                                         RadixTreeNode<K, V>,
                                                         boost::forward_traversal_tag,
                                                         RadixTreeNode<K, V>*>
    {
    public:
        explicit ParentIterator(RadixTreeNode* node) : node(node)
        {
        }

    private:
        friend class boost::iterator_core_access;

        void increment()
        {
            node = node->getNextRealParent();
        }

        bool equal(const ParentIterator& other) const
        {
            return node == other.node;
        }

        RadixTreeNode* dereference() const
        {
            return node;
        }

        RadixTreeNode* node;
    };

    class ParentRange
    {
    public:
        ParentRange(RadixTreeNode* node) : node(node)
        {
        }

        auto begin()
        {
            return ParentIterator<Key, Value>(node->getNextRealParent());
        }

        auto end()
        {
            return ParentIterator<Key, Value>(nullptr);
        }

    private:
        RadixTreeNode* node;
    };

    template <typename KeyType, typename ValueType>
    RadixTreeNode(KeyType&& key, ValueType&& value)
            : key(std::forward<KeyType>(key)), value(std::forward<ValueType>(value))
    {
    }

    RadixTreeNode() : parent(nullptr)
    {
    }

    bool isLeaf() const
    {
        return !children || children->size() == 0;
    }

    const Value& getValue() const
    {
        assert(value);
        return *value;
    }

    Value& getValue()
    {
        assert(value);
        return *value;
    }

    Key& getKey()
    {
        return key;
    }

    ParentRange parents()
    {
        assert(!isInternal());
        return ParentRange(this);
    }

    RadixTreeNode* getNextRealParent() const
    {
        RadixTreeNode* parentNode = parent;
        while (parentNode && parentNode->isInternal()) {
            parentNode = parentNode->parent;
        }
        return parentNode;
    }

    void erase()
    {
        assert(!isInternal());

        if (isRoot()) {
            value = boost::none;
            return;
        }

        RadixTreeNode* parent = this->parent;
        assert(parent->children);

        if (isLeaf()) {
            parent->children->erase(this->key);
        } else if (children->size() == 1) {
            // move child up to this node's parent
            ChildPtr& child = this->children->begin()->second;
            child->key = this->key + child->key;
            parent->insertChild(std::move(child));
            parent->children->erase(this->key);
        } else {
            // more than 1 child
            // make this node an internal one
            value = boost::none;
        }

        // parent of erased node is an internal node with only one child left
        // => move that one child to its grandparent
        if (!parent->isRoot() && parent->isInternal() && parent->children->size() == 1) {
            RadixTreeNode* grandParent = parent->parent;
            ChildPtr& child = parent->children->begin()->second;
            child->key = parent->key + child->key;
            grandParent->insertChild(std::move(child));
            grandParent->children->erase(parent->key);
        }
    }

private:
    template <typename KeyType, typename ValueType>
    RadixTreeNode* addChild(KeyType&& newKey, ValueType&& newValue)
    {
        if (!children) {
            return insertChild(std::make_unique<RadixTreeNode>(
                    std::forward<KeyType>(newKey), std::forward<ValueType>(newValue)));
        }

        // is there already a child whose key starts with the first char of key?
        auto found = children->end();
        for (auto it = children->begin(); it != children->end(); ++it) {
            if (it->second->key.front() == newKey.front()) {
                found = it;
                break;
            }
        }

        // no matching child found, insert new node as it is
        if (found == children->end()) {
            return insertChild(std::make_unique<RadixTreeNode>(
                    std::forward<KeyType>(newKey), std::forward<ValueType>(newValue)));
        }

        ChildPtr& foundNode = found->second;
        // there is already a child with the exact same key, overwrite its value
        if (foundNode->key == newKey) {
            foundNode->value = std::forward<ValueType>(newValue);
            return foundNode.get();
        }

        // new node needs to be inserted in the subtree
        // determine its location
        auto result = std::mismatch(
                newKey.begin(), newKey.end(), foundNode->key.begin(), foundNode->key.end());

        // 1) insert as child of 'foundNode' with the remaining part of key
        if (result.second == foundNode->key.end()) {
            return foundNode->addChild(
                    Key(result.first, newKey.end()), std::forward<ValueType>(newValue));
        }
        // 2) insert as child of current node and parent of 'foundNode'
        else if (result.first == newKey.end()) {
            auto newNode = std::make_unique<RadixTreeNode>(
                    std::forward<KeyType>(newKey), std::forward<ValueType>(newValue));
            foundNode->key = Key(result.second, foundNode->key.end());
            newNode->insertChild(std::move(foundNode));
            children->erase(found);
            return insertChild(std::move(newNode));
        }
        // 3) insert as sibling of 'foundNode'
        else {
            auto newParent =
                    std::make_unique<RadixTreeNode>(Key(newKey.begin(), result.first), boost::none);
            newParent->insertChild(std::make_unique<RadixTreeNode>(
                    Key(result.first, newKey.end()), std::forward<ValueType>(newValue)));
            foundNode->key = Key(result.second, foundNode->key.end());
            newParent->insertChild(std::move(foundNode));
            children->erase(found);
            return insertChild(std::move(newParent));
        }
    }

    RadixTreeNode* insertChild(ChildPtr child)
    {
        if (!children) {
            children.emplace();
        }
        auto emplaceResult = children->emplace(child->key, std::move(child));
        assert(emplaceResult.second);
        auto it = emplaceResult.first;
        ChildPtr& childPtr = it->second;
        childPtr->parent = this;
        return childPtr.get();
    }

    RadixTreeNode* find(const Key& key) const
    {
        if (isLeaf() || key.empty()) {
            return const_cast<RadixTreeNode*>(this);
        }

        for (auto& childIt : *children) {
            const ChildPtr& child = childIt.second;
            if (child->key.front() == key.front()) {
                if (child->key.size() <= key.size()) {
                    return child->find(Key(key.begin() + child->key.size(), key.end()));
                }
                return child.get();
            }
        }
        return const_cast<RadixTreeNode*>(this);
    }

    bool isRoot() const
    {
        return parent == nullptr;
    }

    bool isInternal() const
    {
        return !value;
    }

    template <typename Fun>
    void visit(const Fun& fun, std::vector<std::reference_wrapper<Key>>& parentKeys)
    {
        parentKeys.push_back(key);
        if (!isInternal()) {
            fun(*this, parentKeys);
        }

        if (isLeaf()) {
            parentKeys.pop_back();
            return;
        }

        for (auto& childIt : *children) {
            const ChildPtr& child = childIt.second;
            child->visit(fun, parentKeys);
        }
        parentKeys.pop_back();
    }

    RadixTreeNode* parent;
    Key key;
    boost::optional<Value> value;
    boost::optional<ChildMap> children;
};

template <typename Key, typename Value>
class RadixTree
{

public:
    using Node = RadixTreeNode<Key, Value>;

    RadixTree() : root()
    {
    }

    template <typename KeyType, typename ValueType>
    Node* insert(KeyType&& key, ValueType&& value)
    {
        if (!key.empty()) {
            return root.addChild(std::forward<KeyType>(key), std::forward<ValueType>(value));
        }
        root.value = std::forward<ValueType>(value);
        return &root;
    }

    Node* longestMatch(const Key& key) const
    {
        Node* node = root.find(key);
        while (node->isInternal() && !node->isRoot()) {
            node = node->parent;
        }
        if (node->isRoot() && node->isInternal()) {
            return nullptr;
        }
        return node;
    }

    template <typename Fun>
    void visit(const Fun& fun)
    {
        std::vector<std::reference_wrapper<Key>> keys;
        root.visit(fun, keys);
    }

private:
    RadixTreeNode<Key, Value> root;
};

} // namespace joynr

#endif // ACCESS_CONTROL_RADIXTREE_H
