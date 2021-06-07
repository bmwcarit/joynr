/*
 * #%L
 * %%
 * Copyright (C) 2021 BMW Car IT GmbH
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

import java.util.function.BiConsumer;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

import org.slf4j.Logger;

public abstract class Directory<T> {
    private Set<DirectoryListener<T>> listeners = new HashSet<>();
    private Map<String, T> entryMap = new HashMap<>();

    /**
     * Adds a listener to the directory. The Directory will notify the listeners when an entry is
     * either added or removed by calling their respective entryAdded(...) or entryRemoved(...)
     * callbacks.
     *
     * @param listener the listener to be added
     */
    public synchronized void addListener(DirectoryListener<T> listener) {
        listeners.add(listener);
    }

    /**
     * Removes a listener from the directory.
     *
     * @param listener the listener to be removed
     */
    public synchronized void removeListener(DirectoryListener<T> listener) {
        listeners.remove(listener);
    }

    /**
     * Adds the specified entry and participantId into the directory.
     *
     * @param participantId the entry's participantId
     * @param entry the entry to be added
     */
    public synchronized void add(String participantId, T entry) {
        entryMap.put(participantId, entry);
        for (DirectoryListener<T> listener : listeners) {
            listener.entryAdded(participantId, entry);
        }
    }

    /**
     * Removes the entry with the specified participantId and notifies the listeners via calling their
     * respective entryRemoved methods.
     *
     * @param participantId participantId of the entry supposed to be removed
     * @return the previous entry associated with the participantId or null if the participantId was not present
     */
    public synchronized T remove(String participantId) {
        getLogger().trace("remove: {}", participantId);
        T result = entryMap.remove(participantId);
        if (result == null) {
            getLogger().trace("remove: {} not found", participantId);
        } else {
            for (DirectoryListener<T> listener : listeners) {
                listener.entryRemoved(participantId);
            }
        }
        return result;
    }

    public synchronized T get(String participantId) {
        return entryMap.get(participantId);
    }

    /**
     * Executes the specified consumer on each entry.
     * @param consumer consumer to be executed
     */
    public synchronized void forEach(BiConsumer<String, T> consumer) {
        entryMap.forEach(consumer);
    }

    public synchronized boolean isEmpty() {
        return entryMap.isEmpty();
    }

    protected abstract Logger getLogger();
}
