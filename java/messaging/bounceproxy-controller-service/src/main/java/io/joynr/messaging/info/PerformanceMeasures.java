/*
 * #%L
 * joynr::java::messaging::bounceproxy-controller-service
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
package io.joynr.messaging.info;

import java.io.Serializable;
import java.util.HashMap;
import java.util.Map.Entry;
import java.util.Map;

/**
 * Holds a map of performance measures for bounce proxies.
 * 
 * Performance measures are meant to be simple integer values that represent
 * some kind of load.
 * 
 * @author christina.strobel
 * 
 */
public class PerformanceMeasures implements Serializable {
    private static final long serialVersionUID = 2350423283798167427L;

    public enum Key {

        /**
         * The number of long polls that are handled by the bounce proxy.
         */
        ACTIVE_LONGPOLL_COUNT("activeLongPolls"),

        /**
         * The number of channels the bounce proxy has assigned.
         */
        ASSIGNED_CHANNELS_COUNT("assignedChannels");

        private String name;

        private Key(String name) {
            this.name = name;
        }

        /**
         * Creates a {@link Key} object from a string. This is used for
         * performance measure keys that are handed over e.g. in the query
         * string of a URL where a {@link Key} object as parameter is not
         * possible or not handy.
         * 
         * @param name
         *            the name of the {@link Key} object
         * @return the matching {@link Key} object or <code>null</code> if
         *         there's no such key.
         */
        public static Key fromString(String name) {
            for (Key key : values()) {
                if (key.name.equals(name)) {
                    return key;
                }
            }
            return null;
        }
    }

    private HashMap<Key, Integer> measures = new HashMap<Key, Integer>();

    /**
     * Adds measures from a map with keys and integer values. If a key can't be
     * mapped to a known {@link Key}, the value is ignored without any warning.
     * 
     * @param valueMap
     *   the measures to be added as a Map of string-integer key-value pairs
     */
    public void addMeasures(Map<String, Integer> valueMap) {

        for (Entry<String, Integer> values : valueMap.entrySet()) {

            try {
                addMeasure(values.getKey(), values.getValue().intValue());
            } catch (NumberFormatException e) {
                // TODO for now, we ignore malformed performance
                // measures
            }
        }

    }

    /**
     * Adds a measure. This is a convenient method for
     * {@code addMeasure(k.toString(), i)}. If the key does not exist, for now
     * it is simply ignored without any warning.
     * 
     * @param key
     *            one of the available keys {@link Key} as string.
     * @param value
     *            the value for the specified key
     */
    public void addMeasure(String key, int value) {

        Key k = Key.fromString(key);

        if (k != null) {
            measures.put(k, value);
        } else {
            // TODO for now, we just ignore the value
        }
    }

    /**
     * Adds a measure. If the key is <code>null</code>, the measure is simply
     * ignored without any warning.
     * 
     * @param key the key of the measure to be added to hash map
     * @param value the value of the measure to be added to hash map
     */
    public void addMeasure(Key key, int value) {

        if (key != null) {
            measures.put(key, value);
        } else {
            // TODO for now, we just ignore the value
        }
    }

    /**
     * Returns the value for a measure key.
     * 
     * @param key key for which measure should be returned
     * @return int value for the specified key
     */
    public int getMeasure(Key key) {
        return measures.get(key);
    }

    /**
     * Returns all measures.
     * 
     * @return key-integer hash map of measures
     */
    public HashMap<Key, Integer> getMeasures() {
        return measures;
    }

    /* (non-Javadoc)
     * @see java.lang.Object#equals(java.lang.Object)
     */
    @Override
    public boolean equals(Object obj) {

        if (this == obj) {
            return true;
        }

        if (!(obj instanceof PerformanceMeasures)) {
            return false;
        }

        PerformanceMeasures p = (PerformanceMeasures) obj;

        return p.measures.equals(measures);
    }

    /*
     * (non-Javadoc)
     * 
     * @see java.lang.Object#hashCode()
     */
    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + ((measures == null) ? 0 : measures.hashCode());
        return result;
    }

    @Override
    public String toString() {
        return "PerformanceMeasures [measures=" + measures + "]";
    }

}
