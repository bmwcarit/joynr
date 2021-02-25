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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.runners.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class MultiMapTest {

    @Test
    public void storesMultipleValuesPerKey() {
        MultiMap<String, String> multiMap = new MultiMap<>();
        final String key1 = "key1";
        final String key2 = "key2";
        final Set<String> keySet = new HashSet<>(Arrays.asList(key1, key2));
        final Set<String> values1 = new HashSet<>(Arrays.asList("values1_1", "values1_2, values1_3"));
        final Set<String> values2 = new HashSet<>(Arrays.asList("values2_1", "values2_2, values2_3"));
        values1.forEach(v -> multiMap.put(key1, v));
        values2.forEach(v -> multiMap.put(key2, v));
        assertEquals(values1, multiMap.get(key1));
        assertEquals(values2, multiMap.get(key2));
        assertEquals(keySet, multiMap.keySet());
    }

    @Test
    public void containsKeyAfterPut() {
        MultiMap<String, String> multiMap = new MultiMap<>();
        final String key = "key";
        final String value = "value";
        multiMap.put(key, value);
        assertTrue(multiMap.containsKey(key));
    }

    @Test
    public void emptyMapDoesNotContainKey() {
        MultiMap<String, String> multiMap = new MultiMap<>();
        final String key = "key";
        assertFalse(multiMap.containsKey(key));
    }

    @Test
    public void emptyMapIsEmpty() {
        MultiMap<String, String> multiMap = new MultiMap<>();
        assertTrue(multiMap.isEmpty());
    }

    @Test
    public void mapIsNotEmptyAfterPut() {
        MultiMap<String, String> multiMap = new MultiMap<>();
        multiMap.put("key", "value");
        assertFalse(multiMap.isEmpty());
    }

    @Test
    public void emptyMapKeySetIsEmpty() {
        MultiMap<String, String> multiMap = new MultiMap<>();
        assertTrue(multiMap.keySet().isEmpty());
    }

    @Test
    public void getForNonExisitingKeyReturnsEmptySet() {
        MultiMap<String, String> multiMap = new MultiMap<>();
        final String key = "key";
        Set<String> values = multiMap.get(key);
        assertTrue(values.isEmpty());
    }

    @Test
    public void getAndRemoveAllForNonExisitingKeyReturnsEmptySet() {
        MultiMap<String, String> multiMap = new MultiMap<>();
        final String key = "key";
        final String value = "value";
        final String nonExistedKey = "nonExistedKey";
        multiMap.put(key, value);
        assertEquals(1, multiMap.size());

        Set<String> values = multiMap.getAndRemoveAll(nonExistedKey);

        assertTrue(values.isEmpty());
        assertEquals(1, multiMap.size());
    }

    @Test
    public void getAndRemoveAllForExisitingKeyReturnsSetThenRemoveItFromTheBaseMap() {
        MultiMap<String, String> multiMap = new MultiMap<>();
        final String key = "key";
        final String value1 = "value1";
        final String value2 = "value2";
        final String value3 = "value3";
        multiMap.put(key, value1);
        multiMap.put(key, value2);
        multiMap.put(key, value3);

        assertTrue(multiMap.containsKey(key));
        assertEquals(3, multiMap.size());

        Set<String> values = multiMap.getAndRemoveAll(key);

        assertEquals(3, values.size());
        assertEquals(0, multiMap.size());
    }

    @Test
    public void removeNonExistingKeyValuePairReturnsFalse() {
        MultiMap<String, String> multiMap = new MultiMap<>();
        final String key = "key";
        final String value1 = "value1";
        final String value2 = "value2";
        assertFalse(multiMap.remove(key, value1));
        multiMap.put(key, value1);
        assertFalse(multiMap.remove(key, value2));
    }

    @Test
    public void removeExistingKeyValuePairReturnsTrue() {
        MultiMap<String, String> multiMap = new MultiMap<>();
        final String key = "key";
        final String value = "value";
        multiMap.put(key, value);
        assertTrue(multiMap.remove(key, value));
    }

    @Test
    public void doesNotContainKeyAfterRemove() {
        MultiMap<String, String> multiMap = new MultiMap<>();
        final String key = "key";
        final String value = "value";
        multiMap.put(key, value);
        multiMap.remove(key, value);
        assertFalse(multiMap.containsKey(key));
    }

    @Test
    public void stillContainsKeyAfterPartialRemove() {
        MultiMap<String, String> multiMap = new MultiMap<>();
        final String key = "key";
        final String value1 = "value_1";
        final String value2 = "value_2";
        multiMap.put(key, value1);
        multiMap.put(key, value2);
        multiMap.remove(key, value1);
        assertTrue(multiMap.containsKey(key));
    }

    @Test
    public void doesNotContainKeyAfterRemoveAll() {
        MultiMap<String, String> multiMap = new MultiMap<>();
        final String key = "key";
        final Set<String> values = new HashSet<>(Arrays.asList("value1", "value2, value3"));
        values.forEach(v -> multiMap.put(key, v));
        multiMap.removeAll(key);
        assertFalse(multiMap.containsKey(key));
    }

    @Test
    public void emptyMapHasSizeZero() {
        MultiMap<String, String> multiMap = new MultiMap<>();
        assertEquals(0, multiMap.size());
    }

    @Test
    public void mapSizeGrowsAndShrinks() {
        MultiMap<String, String> multiMap = new MultiMap<>();
        final String key = "key";
        final Set<String> values = new HashSet<>(Arrays.asList("value1", "value2, value3"));
        int size = 0;
        for (String v : values) {
            multiMap.put(key, v);
            assertEquals(++size, multiMap.size());
        }
        for (String v : values) {
            multiMap.remove(key, v);
            assertEquals(--size, multiMap.size());
        }
    }
}
