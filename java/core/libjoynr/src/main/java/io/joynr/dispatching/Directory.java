/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
package io.joynr.dispatching;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map.Entry;
import java.util.Map;
import java.util.Set;

import org.slf4j.Logger;

public abstract class Directory<T> {
    protected Set<DirectoryListener<T>> listeners = new HashSet<>();
    protected Map<String, T> entryMap = new HashMap<>();

    public void addListener(DirectoryListener<T> listener) {
        synchronized (entryMap) {
            listeners.add(listener);
            for (Entry<String, T> entry : entryMap.entrySet()) {
                listener.entryAdded(entry.getKey(), entry.getValue());
            }
        }
    }

    public void removeListener(DirectoryListener<T> listener) {
        synchronized (entryMap) {
            listeners.remove(listener);
        }
    }

    public void add(String id, T entry) {
        synchronized (entryMap) {
            entryMap.put(id, entry);
            for (DirectoryListener<T> listener : listeners) {
                listener.entryAdded(id, entry);
            }
        }
    }

    public T remove(String id) {
        synchronized (entryMap) {
            getLogger().trace("remove: {}", id);
            T result = entryMap.remove(id);
            if (result == null) {
                getLogger().trace("remove: {} not found", id);
            } else {
                for (DirectoryListener<T> listener : listeners) {
                    listener.entryRemoved(id);
                }
            }
            return result;
        }
    }

    public T get(String id) {
        return entryMap.get(id);
    }

    public boolean isEmpty() {
        return entryMap.isEmpty();
    }

    protected abstract Logger getLogger();
}
