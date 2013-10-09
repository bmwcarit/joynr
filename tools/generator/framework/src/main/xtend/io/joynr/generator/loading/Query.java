package io.joynr.generator.loading;

/*
 * #%L
 * joynr::tools::generator::joynr Generator Framework
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
 * %%
 * __________________
 * 
 * NOTICE:  Dissemination of this information or reproduction of this material 
 * is strictly  forbidden unless prior written permission is obtained from 
 * BMW Car IT GmbH.
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
