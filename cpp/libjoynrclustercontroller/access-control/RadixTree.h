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
        explicit ParentIterator(RadixTreeNode* node) : _node(node)
        {
        }

    private:
        friend class boost::iterator_core_access;

        void increment()
        {
            _node = _node->getNextRealParent();
        }

        bool equal(const ParentIterator& other) const
        {
            return _node == other._node;
        }

        RadixTreeNode* dereference() const
        {
            return _node;
        }

        RadixTreeNode* _node;
    };

    class ParentRange
    {
    public:
        ParentRange(RadixTreeNode* node) : _node(node)
        {
        }

        auto begin()
        {
            return ParentIterator<Key, Value>(_node->getNextRealParent());
        }

        auto end()
        {
            return ParentIterator<Key, Value>(nullptr);
        }

    private:
        RadixTreeNode* _node;
    };

    template <typename KeyType, typename ValueType>
    RadixTreeNode(KeyType&& key, ValueType&& value)
            : _key(std::forward<KeyType>(key)), _value(std::forward<ValueType>(value))
    {
    }

    RadixTreeNode() : _parent(nullptr)
    {
    }

    Key getFullKey() const
    {
        auto currentNode = this;
        Key fullKey = currentNode->_key;
        while (!currentNode->isRoot()) {
            currentNode = currentNode->_parent;
            fullKey = currentNode->_key + fullKey;
        }
        return fullKey;
    }

    bool isLeaf() const
    {
        return !_children || _children->size() == 0;
    }

    const Value& getValue() const
    {
        assert(_value);
        return *_value;
    }

    Value& getValue()
    {
        assert(_value);
        return *_value;
    }

    Key& getKey()
    {
        return _key;
    }

    ParentRange parents()
    {
        assert(!isInternal());
        return ParentRange(this);
    }

    RadixTreeNode* getNextRealParent() const
    {
        RadixTreeNode* parentNode = _parent;
        while (parentNode && parentNode->isInternal()) {
            parentNode = parentNode->_parent;
        }
        return parentNode;
    }

    void erase()
    {
        assert(!isInternal());

        if (isRoot()) {
            _value = boost::none;
            return;
        }

        RadixTreeNode* parent = this->_parent;
        assert(parent->_children);

        if (isLeaf()) {
            parent->_children->erase(this->_key);
        } else if (_children->size() == 1) {
            // move child up to this node's parent
            ChildPtr& child = this->_children->begin()->second;
            child->_key = this->_key + child->_key;
            parent->insertChild(std::move(child));
            parent->_children->erase(this->_key);
        } else {
            // more than 1 child
            // make this node an internal one
            _value = boost::none;
        }

        // parent of erased node is an internal node with only one child left
        // => move that one child to its grandparent
        if (!parent->isRoot() && parent->isInternal() && parent->_children->size() == 1) {
            RadixTreeNode* grandParent = parent->_parent;
            ChildPtr& child = parent->_children->begin()->second;
            child->_key = parent->_key + child->_key;
            grandParent->insertChild(std::move(child));
            grandParent->_children->erase(parent->_key);
        }
    }

private:
    template <typename KeyType, typename ValueType>
    RadixTreeNode* addChild(KeyType&& newKey, ValueType&& newValue)
    {
        if (!_children) {
            return insertChild(std::make_unique<RadixTreeNode>(
                    std::forward<KeyType>(newKey), std::forward<ValueType>(newValue)));
        }

        // is there already a child whose key starts with the first char of key?
        auto found = _children->end();
        for (auto it = _children->begin(); it != _children->end(); ++it) {
            if (it->second->_key.front() == newKey.front()) {
                found = it;
                break;
            }
        }

        // no matching child found, insert new node as it is
        if (found == _children->end()) {
            return insertChild(std::make_unique<RadixTreeNode>(
                    std::forward<KeyType>(newKey), std::forward<ValueType>(newValue)));
        }

        ChildPtr& foundNode = found->second;
        // there is already a child with the exact same key, overwrite its value
        if (foundNode->_key == newKey) {
            foundNode->_value = std::forward<ValueType>(newValue);
            return foundNode.get();
        }

        // new node needs to be inserted in the subtree
        // determine its location
        auto result = std::mismatch(
                newKey.begin(), newKey.end(), foundNode->_key.begin(), foundNode->_key.end());

        // 1) insert as child of 'foundNode' with the remaining part of key
        if (result.second == foundNode->_key.end()) {
            return foundNode->addChild(
                    Key(result.first, newKey.end()), std::forward<ValueType>(newValue));
        }
        // 2) insert as child of current node and parent of 'foundNode'
        else if (result.first == newKey.end()) {
            auto newNode = std::make_unique<RadixTreeNode>(
                    std::forward<KeyType>(newKey), std::forward<ValueType>(newValue));
            foundNode->_key = Key(result.second, foundNode->_key.end());
            newNode->insertChild(std::move(foundNode));
            _children->erase(found);
            return insertChild(std::move(newNode));
        }
        // 3) insert as sibling of 'foundNode'
        else {
            auto newParent =
                    std::make_unique<RadixTreeNode>(Key(newKey.begin(), result.first), boost::none);
            newParent->insertChild(std::make_unique<RadixTreeNode>(
                    Key(result.first, newKey.end()), std::forward<ValueType>(newValue)));
            foundNode->_key = Key(result.second, foundNode->_key.end());
            newParent->insertChild(std::move(foundNode));
            _children->erase(found);
            return insertChild(std::move(newParent));
        }
    }

    RadixTreeNode* insertChild(ChildPtr child)
    {
        if (!_children) {
            _children.emplace();
        }
        auto emplaceResult = _children->emplace(child->_key, std::move(child));
        assert(emplaceResult.second);
        auto it = emplaceResult.first;
        ChildPtr& childPtr = it->second;
        childPtr->_parent = this;
        return childPtr.get();
    }

    RadixTreeNode* find(const Key& key) const
    {
        if (isLeaf() || key.empty()) {
            return const_cast<RadixTreeNode*>(this);
        }

        for (auto& childIt : *_children) {
            const ChildPtr& child = childIt.second;
            if (child->_key.front() == key.front()) {
                if (child->_key.size() <= key.size() &&
                    key.compare(0, child->_key.size(), child->_key) == 0) {
                    return child->find(
                            Key(key.begin() + static_cast<std::int64_t>(child->_key.size()),
                                key.end()));
                }
            }
        }
        return const_cast<RadixTreeNode*>(this);
    }

    bool isRoot() const
    {
        return _parent == nullptr;
    }

    bool isInternal() const
    {
        return !_value;
    }

    template <typename Fun>
    void visit(const Fun& fun, std::vector<std::reference_wrapper<Key>>& parentKeys)
    {
        parentKeys.push_back(_key);
        if (!isInternal()) {
            fun(*this, parentKeys);
        }

        if (isLeaf()) {
            parentKeys.pop_back();
            return;
        }

        for (auto& childIt : *_children) {
            const ChildPtr& child = childIt.second;
            child->visit(fun, parentKeys);
        }
        parentKeys.pop_back();
    }

    RadixTreeNode* _parent;
    Key _key;
    boost::optional<Value> _value;
    boost::optional<ChildMap> _children;
};

template <typename Key, typename Value>
class RadixTree
{

public:
    using Node = RadixTreeNode<Key, Value>;

    RadixTree() : _root()
    {
    }

    template <typename KeyType, typename ValueType>
    Node* insert(KeyType&& key, ValueType&& value)
    {
        if (!key.empty()) {
            return _root.addChild(std::forward<KeyType>(key), std::forward<ValueType>(value));
        }
        _root._value = std::forward<ValueType>(value);
        return &_root;
    }

    Node* longestMatch(const Key& key) const
    {
        Node* node = _root.find(key);
        while (node->isInternal() && !node->isRoot()) {
            node = node->_parent;
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
        _root.visit(fun, keys);
    }

private:
    RadixTreeNode<Key, Value> _root;
};

} // namespace joynr

#endif // ACCESS_CONTROL_RADIXTREE_H
