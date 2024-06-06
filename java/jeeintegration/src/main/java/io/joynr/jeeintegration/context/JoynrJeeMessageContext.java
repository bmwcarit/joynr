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
package io.joynr.jeeintegration.context;

import java.lang.annotation.Annotation;
import java.util.HashMap;
import java.util.Map;

import jakarta.enterprise.context.ContextNotActiveException;
import jakarta.enterprise.context.spi.Context;
import jakarta.enterprise.context.spi.Contextual;
import jakarta.enterprise.context.spi.CreationalContext;

import io.joynr.jeeintegration.api.JoynrJeeMessageScoped;

/**
 * The {@link Context} which holds {@link JoynrJeeMessageScoped} {@link Contextual contextual instances} for the
 * duration of a joynr message being processed.
 */
public class JoynrJeeMessageContext implements Context {

    private static JoynrJeeMessageContext instance = new JoynrJeeMessageContext();

    private ThreadLocal<Map<Contextual<?>, Object>> contextualStore = new ThreadLocal<>();

    @Override
    public Class<? extends Annotation> getScope() {
        return JoynrJeeMessageScoped.class;
    }

    @Override
    public <T> T get(Contextual<T> contextual, CreationalContext<T> creationalContext) {
        T instance = get(contextual);
        if (instance == null) {
            instance = contextual.create(creationalContext);
            if (instance != null) {
                contextualStore.get().put(contextual, instance);
            }
        }
        return instance;
    }

    @SuppressWarnings("unchecked")
    @Override
    public <T> T get(Contextual<T> contextual) {
        Map<Contextual<?>, Object> store = contextualStore.get();
        if (store == null) {
            throw new ContextNotActiveException();
        }
        return (T) store.get(contextual);
    }

    @Override
    public boolean isActive() {
        return contextualStore.get() != null;
    }

    public void activate() {
        contextualStore.set(new HashMap<>());
    }

    public void deactivate() {
        if (contextualStore.get() != null) {
            contextualStore.get().clear();
        }
        contextualStore.remove();
    }

    public static JoynrJeeMessageContext getInstance() {
        return instance;
    }

}
