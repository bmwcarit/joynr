package io.joynr.context;

import java.util.HashMap;

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

import java.util.Map;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Key;
import com.google.inject.Provider;
import com.google.inject.Scope;

/**
 * Implementation of the joynr message scope, for instances which are to be valid during the processing of joynr
 * messages and are marked as {@link JoynrMessageScoped}.
 */
public class JoynrMessageScope implements Scope {

    private static final Logger logger = LoggerFactory.getLogger(JoynrMessageScope.class);

    private ThreadLocal<Map<Key<?>, Object>> entries = new ThreadLocal<>();

    @Override
    public <T> Provider<T> scope(final Key<T> key, final Provider<T> unscoped) {
        logger.trace("Called with {} and {}", key, unscoped);
        return new Provider<T>() {
            @Override
            public T get() {
                Map<Key<?>, Object> scopeObject = entries.get();
                if (scopeObject == null) {
                    throw new IllegalStateException("Scope " + JoynrMessageScope.class.getSimpleName() + " not active.");
                }
                logger.trace("Get called on scoped provider for {}", key);
                @SuppressWarnings("unchecked")
                T result = (T) scopeObject.get(key);
                if (result == null) {
                    result = unscoped.get();
                    scopeObject.put(key, result);
                }
                logger.trace("Returning instance {} for key {}", result, key);
                return result;
            }
        };
    }

    /**
     * Call this method to activate the {@link JoynrMessageScope} for the current thread. It is not valid to call this
     * method more than once.
     *
     * @throws IllegalStateException
     *             if you have already activated this scope for the current thread.
     */
    public void activate() {
        if (entries.get() != null) {
            throw new IllegalStateException("Scope " + JoynrMessageScope.class.getSimpleName() + " is already active.");
        }
        logger.trace("Activating {} scope", JoynrMessageScope.class.getSimpleName());
        entries.set(new HashMap<Key<?>, Object>());
    }

    /**
     * Call this method in a <code>finally</code> block after having called {@link #activate()} in order to de-activate
     * the scope for the current thread.
     */
    public void deactivate() {
        if (entries.get() == null) {
            throw new IllegalStateException("Scope " + JoynrMessageScope.class.getSimpleName()
                    + " is not currently active. Can't deactivate.");
        }
        logger.trace("Deactivating {} scope", JoynrMessageScope.class.getSimpleName());
        entries.remove();
    }

}
