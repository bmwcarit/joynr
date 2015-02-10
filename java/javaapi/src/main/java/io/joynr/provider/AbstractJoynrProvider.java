package io.joynr.provider;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

import io.joynr.pubsub.publication.AttributeListener;
import io.joynr.pubsub.publication.BroadcastFilter;
import io.joynr.pubsub.publication.BroadcastFilterImpl;
import io.joynr.pubsub.publication.BroadcastListener;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.ConcurrentHashMap;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public abstract class AbstractJoynrProvider implements JoynrProvider {
    private static final Logger LOG = LoggerFactory.getLogger(AbstractJoynrProvider.class);

    ConcurrentHashMap<String, List<AttributeListener>> attributeListeners;
    ConcurrentHashMap<String, List<BroadcastListener>> broadcastListeners;
    protected ConcurrentHashMap<String, List<BroadcastFilter>> broadcastFilters;

    public AbstractJoynrProvider() {
        attributeListeners = new ConcurrentHashMap<String, List<AttributeListener>>();
        broadcastListeners = new ConcurrentHashMap<String, List<BroadcastListener>>();
        broadcastFilters = new ConcurrentHashMap<String, List<BroadcastFilter>>();
    }

    public void onAttributeValueChanged(String attributeName, Object value) {
        if (!attributeListeners.containsKey(attributeName)) {
            return;
        }
        List<AttributeListener> listeners = attributeListeners.get(attributeName);
        synchronized (listeners) {
            for (AttributeListener listener : listeners) {
                listener.attributeValueChanged(value);
            }
        }
    }

    public void fireBroadcast(String broadcastName, List<BroadcastFilter> broadcastFilters, Object... values) {
        if (!broadcastListeners.containsKey(broadcastName)) {
            return;
        }
        List<BroadcastListener> listeners = broadcastListeners.get(broadcastName);
        synchronized (listeners) {
            for (BroadcastListener listener : listeners) {
                listener.broadcastOccurred(broadcastFilters, values);
            }
        }
    }

    @Override
    public void registerAttributeListener(String attributeName, AttributeListener attributeListener) {
        attributeListeners.putIfAbsent(attributeName, new ArrayList<AttributeListener>());
        List<AttributeListener> listeners = attributeListeners.get(attributeName);
        synchronized (listeners) {
            listeners.add(attributeListener);
        }
    }

    @Override
    public void unregisterAttributeListener(String attributeName, AttributeListener attributeListener) {
        List<AttributeListener> listeners = attributeListeners.get(attributeName);
        if (listeners == null) {
            LOG.error("trying to unregister an attribute listener for attribute \"" + attributeName
                    + "\" that was never registered");
            return;
        }
        synchronized (listeners) {
            boolean success = listeners.remove(attributeListener);
            if (!success) {
                LOG.error("trying to unregister an attribute listener for attribute \"" + attributeName
                        + "\" that was never registered");
                return;
            }
        }
    }

    @Override
    public void registerBroadcastListener(String broadcastName, BroadcastListener broadcastListener) {
        broadcastListeners.putIfAbsent(broadcastName, new ArrayList<BroadcastListener>());
        List<BroadcastListener> listeners = broadcastListeners.get(broadcastName);
        synchronized (listeners) {
            listeners.add(broadcastListener);
        }
    }

    @Override
    public void unregisterBroadcastListener(String broadcastName, BroadcastListener broadcastListener) {
        List<BroadcastListener> listeners = broadcastListeners.get(broadcastName);
        if (listeners == null) {
            LOG.error("trying to unregister a listener for broadcast \"" + broadcastName
                    + "\" that was never registered");
            return;
        }
        synchronized (listeners) {
            boolean success = listeners.remove(broadcastListener);
            if (!success) {
                LOG.error("trying to unregister a listener for broadcast \"" + broadcastName
                        + "\" that was never registered");
                return;
            }
        }
    }

    public void addBroadcastFilter(BroadcastFilterImpl filter) {
        if (broadcastFilters.containsKey(filter.getName())) {
            broadcastFilters.get(filter.getName()).add(filter);
        } else {
            ArrayList<BroadcastFilter> list = new ArrayList<BroadcastFilter>();
            list.add(filter);
            broadcastFilters.put(filter.getName(), list);
        }
    }

    public void addBroadcastFilter(BroadcastFilterImpl... filters) {
        List<BroadcastFilterImpl> filtersList = Arrays.asList(filters);

        for (BroadcastFilterImpl filter : filtersList) {
            addBroadcastFilter(filter);
        }
    }

}
