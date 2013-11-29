package io.joynr.generator.loading;

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

import org.eclipse.emf.ecore.EObject;
import org.eclipse.xtext.util.SimpleAttributeResolver;

import com.google.common.base.Predicate;
import com.google.common.collect.Iterables;

public class Query {

    private final Iterable<EObject> content;

    public Query(Iterable<EObject> content) {
        this.content = content;
    }

    public static Query query(Iterable<EObject> content) {
        return new Query(content);
    }

    public <T extends EObject> T find(String name, Class<T> type) {
        return Iterables.find(all(type), name(name));
    }

    private <T extends EObject> Predicate<T> name(final String name) {
        return new Predicate<T>() {

            public boolean apply(T input) {
                String candidate = SimpleAttributeResolver.NAME_RESOLVER.apply(input);
                return name.equals(candidate);
            }
        };
    }

    public <T> T first(Class<T> type) {
        return type.cast(all(type).iterator().next());
    }

    public EObject find(Predicate<EObject> predicate) {
        return Iterables.find(content, predicate);
    }

    public <T> Iterable<T> all(Class<T> type) {
        return Iterables.filter(content, type);
    }
}
