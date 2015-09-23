package io.joynr.dispatching;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;

import net.sf.ehcache.util.concurrent.ConcurrentHashMap;

import org.slf4j.Logger;

import com.google.common.collect.Sets;

public abstract class CallerDirectory<T> {
    protected Set<CallerDirectoryListener<T>> listeners = Sets.newHashSet();
    protected Map<String, T> callers = new ConcurrentHashMap<String, T>();

    public void addListener(CallerDirectoryListener<T> listener) {
        synchronized (callers) {
            listeners.add(listener);
            for (Entry<String, T> entry : callers.entrySet()) {
                listener.callerAdded(entry.getKey(), entry.getValue());
            }
        }
    }

    public void removeListener(CallerDirectoryListener<T> listener) {
        synchronized (callers) {
            listeners.remove(listener);
        }
    }

    public void addCaller(String id, T caller) {
        synchronized (callers) {
            callers.put(id, caller);
            for (CallerDirectoryListener<T> listener : listeners) {
                listener.callerAdded(id, caller);
            }
        }
    }

    public T removeCaller(String id) {
        synchronized (callers) {
            getLogger().trace("removeCaller: {}", id);
            T result = callers.remove(id);
            if (result == null) {
                getLogger().trace("removeCaller: {} not found", id);
            } else {
                for (CallerDirectoryListener<T> listener : listeners) {
                    listener.callerRemoved(id);
                }
            }
            return result;
        }
    }

    public boolean containsCaller(String id) {
        return callers.containsKey(id);
    }

    public T getCaller(String id) {
        return callers.get(id);
    }

    public boolean isEmpty() {
        return callers.isEmpty();
    }

    protected abstract Logger getLogger();
}
