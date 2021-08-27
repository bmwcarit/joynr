/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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

import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.locks.ReentrantReadWriteLock;

import com.fasterxml.jackson.core.JsonParseException;
import com.fasterxml.jackson.core.JsonParser;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.core.TreeNode;
import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.DeserializationConfig;
import com.fasterxml.jackson.databind.DeserializationFeature;
import com.fasterxml.jackson.databind.JavaType;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.MapperFeature;
import com.fasterxml.jackson.databind.Module;
import com.fasterxml.jackson.databind.ObjectMapper.DefaultTyping;
import com.fasterxml.jackson.databind.SerializationConfig;
import com.fasterxml.jackson.databind.SerializationFeature;

public class ObjectMapper {

    private final com.fasterxml.jackson.databind.ObjectMapper realObjectMapper;
    private final Set<Class<?>> registeredSubtypes;
    private final ReentrantReadWriteLock lock;

    public ObjectMapper() {
        this(new com.fasterxml.jackson.databind.ObjectMapper());
    }

    ObjectMapper(com.fasterxml.jackson.databind.ObjectMapper realObjectMapper) {
        Objects.requireNonNull(realObjectMapper);
        this.realObjectMapper = realObjectMapper;
        registeredSubtypes = new HashSet<Class<?>>();
        lock = new ReentrantReadWriteLock();
    }

    public <T> T readValue(JsonParser p, Class<T> valueType) throws IOException {
        lock.readLock().lock();
        try {
            return realObjectMapper.readValue(p, valueType);
        } finally {
            lock.readLock().unlock();
        }
    }

    public <T> T readValue(URL src, Class<T> valueType) throws IOException, JsonParseException, JsonMappingException {
        lock.readLock().lock();
        try {
            return realObjectMapper.readValue(src, valueType);
        } finally {
            lock.readLock().unlock();
        }
    }

    public <T> T readValue(String content, Class<T> valueType) throws JsonProcessingException, JsonMappingException {
        lock.readLock().lock();
        try {
            return realObjectMapper.readValue(content, valueType);
        } finally {
            lock.readLock().unlock();
        }
    }

    public <T> T readValue(String content, TypeReference<T> valueTypeRef) throws JsonProcessingException,
                                                                          JsonMappingException {
        lock.readLock().lock();
        try {
            return realObjectMapper.readValue(content, valueTypeRef);
        } finally {
            lock.readLock().unlock();
        }
    }

    public <T> T treeToValue(TreeNode n, Class<T> valueType) throws IllegalArgumentException, JsonProcessingException {
        lock.readLock().lock();
        try {
            return realObjectMapper.treeToValue(n, valueType);
        } finally {
            lock.readLock().unlock();
        }
    }

    public <T extends JsonNode> T valueToTree(Object fromValue) throws IllegalArgumentException {
        lock.readLock().lock();
        try {
            return realObjectMapper.valueToTree(fromValue);
        } finally {
            lock.readLock().unlock();
        }
    }

    public String writeValueAsString(Object value) throws JsonProcessingException {
        lock.readLock().lock();
        try {
            return realObjectMapper.writeValueAsString(value);
        } finally {
            lock.readLock().unlock();
        }
    }

    public <T> T convertValue(Object fromValue, Class<T> toValueType) throws IllegalArgumentException {
        lock.readLock().lock();
        try {
            return realObjectMapper.convertValue(fromValue, toValueType);
        } finally {
            lock.readLock().unlock();
        }
    }

    public <T> T convertValue(Object fromValue, TypeReference<T> toValueTypeRef) throws IllegalArgumentException {
        lock.readLock().lock();
        try {
            return realObjectMapper.convertValue(fromValue, toValueTypeRef);
        } finally {
            lock.readLock().unlock();
        }
    }

    public <T> T convertValue(Object fromValue, JavaType toValueType) throws IllegalArgumentException {
        lock.readLock().lock();
        try {
            return realObjectMapper.convertValue(fromValue, toValueType);
        } finally {
            lock.readLock().unlock();
        }
    }

    @Deprecated
    public void enableDefaultTypingAsProperty(DefaultTyping applicability, String propertyName) {
        lock.writeLock().lock();
        try {
            realObjectMapper.enableDefaultTypingAsProperty(applicability, propertyName);
        } finally {
            lock.writeLock().unlock();
        }
    }

    public void configure(SerializationFeature f, boolean state) {
        lock.writeLock().lock();
        try {
            realObjectMapper.configure(f, state);
        } finally {
            lock.writeLock().unlock();
        }
    }

    public void configure(DeserializationFeature f, boolean state) {
        lock.writeLock().lock();
        try {
            realObjectMapper.configure(f, state);
        } finally {
            lock.writeLock().unlock();
        }
    }

    public void configure(MapperFeature f, boolean state) {
        lock.writeLock().lock();
        try {
            realObjectMapper.configure(f, state);
        } finally {
            lock.writeLock().unlock();
        }
    }

    public SerializationConfig getSerializationConfig() {
        lock.readLock().lock();
        try {
            return realObjectMapper.getSerializationConfig();
        } finally {
            lock.readLock().unlock();
        }
    }

    public DeserializationConfig getDeserializationConfig() {
        lock.readLock().lock();
        try {
            return realObjectMapper.getDeserializationConfig();
        } finally {
            lock.readLock().unlock();
        }
    }

    public void registerModule(Module module) {
        lock.writeLock().lock();
        try {
            realObjectMapper.registerModule(module);
        } finally {
            lock.writeLock().unlock();
        }
    }

    public void registerSubtypes(Class<?>... classes) {
        lock.readLock().lock();
        try {
            classes = Arrays.asList(classes)
                            .stream()
                            .filter(c -> !registeredSubtypes.contains(c))
                            .toArray(size -> new Class<?>[size]);
        } finally {
            lock.readLock().unlock();
        }
        if (0 != classes.length) {
            lock.writeLock().lock();
            try {
                realObjectMapper.registerSubtypes(classes);
                registeredSubtypes.addAll(Arrays.asList(classes));
            } finally {
                lock.writeLock().unlock();
            }
        }
    }

    public <T> T readValue(byte[] src, Class<T> valueType) throws IOException, JsonParseException,
                                                           JsonMappingException {
        lock.readLock().lock();
        try {
            return realObjectMapper.readValue(src, valueType);
        } finally {
            lock.readLock().unlock();
        }
    }

    public JsonNode readTree(InputStream in) throws IOException {
        lock.readLock().lock();
        try {
            return realObjectMapper.readTree(in);
        } finally {
            lock.readLock().unlock();
        }
    }
}
