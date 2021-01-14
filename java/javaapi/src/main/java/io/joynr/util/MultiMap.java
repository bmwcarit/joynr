/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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
package io.joynr.util;

import java.io.Serializable;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

public class MultiMap<K, V> implements Serializable {
    private final Map<K, Set<V>> storage = new HashMap<>();

    /**
     * @param key key
     * @return {@code true} if at least one key-value pair exists
     * for {@code key}
     */
    public boolean containsKey(K key) {
        return storage.containsKey(key);
    }

    /**
     * @return {@code true} if empty
     */
    public boolean isEmpty() {
        return storage.isEmpty();
    }

    /**
     * @param key key
     * @return a set view of the values associated with {@code key}
     */
    public Set<V> get(K key) {
        if (!containsKey(key)) {
            return new HashSet<V>();
        }
        return storage.get(key);
    }

    /**
     * Stores a {@code key}-{@code value} pair.
     * @param key key
     * @param value value
     */
    public void put(K key, V value) {
        Set<V> set = storage.get(key);
        if (set == null) {
            set = new HashSet<>();
            storage.put(key, set);
        }
        set.add(value);
    }

    /**
     * Removes {@code key}-{@code value} pair if it exists.
     *
     * @param key key
     * @param value value
     * @return {@code true} if pair was removed, {@code false} otherwise
     */
    public boolean remove(K key, V value) {
        Set<V> set = storage.get(key);
        if (set != null) {
            final boolean removed = set.remove(value);
            if (removed && set.isEmpty()) {
                storage.remove(key);
            }
            return removed;
        }
        return false;
    }

    /**
     * Removes all values associated with {@code key}.
     * @param key key
     */
    public void removeAll(K key) {
        storage.remove(key);
    }

    /**
     * @return a set view of the keys contained in this multimap
     */
    public Set<K> keySet() {
        return storage.keySet();
    }

    /**
     * @return number of key-value pairs in this multimap
     */
    public int size() {
        return this.storage.values().stream().mapToInt(Set::size).sum();
    }
}
