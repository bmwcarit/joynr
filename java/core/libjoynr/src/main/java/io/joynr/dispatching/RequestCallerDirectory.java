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

import javax.inject.Singleton;

import net.sf.ehcache.util.concurrent.ConcurrentHashMap;

import com.google.common.collect.Sets;

@Singleton
public class RequestCallerDirectory {
    private Set<RequestCallerDirectoryListener> listeners = Sets.newHashSet();
    private Map<String, RequestCaller> callers = new ConcurrentHashMap<String, RequestCaller>();

    public void addListener(RequestCallerDirectoryListener listener) {
        synchronized (callers) {
            listeners.add(listener);
            for (Entry<String, RequestCaller> entry : callers.entrySet()) {
                listener.requestCallerAdded(entry.getKey(), entry.getValue());
            }
        }
    }

    public void removeListener(RequestCallerDirectoryListener listener) {
        synchronized (callers) {
            listeners.remove(listener);
        }
    }

    public void addRequestCaller(String participantId, RequestCaller caller) {
        synchronized (callers) {
            callers.put(participantId, caller);
            for (RequestCallerDirectoryListener listener : listeners) {
                listener.requestCallerAdded(participantId, caller);
            }
        }
    }

    public void removeRequestCaller(String participantId) {
        synchronized (callers) {
            callers.remove(participantId);
            for (RequestCallerDirectoryListener listener : listeners) {
                listener.requestCallerRemoved(participantId);
            }
        }
    }

    public boolean containsKey(String participantId) {
        return callers.containsKey(participantId);
    }

    public RequestCaller get(String participantId) {
        return callers.get(participantId);
    }
}
