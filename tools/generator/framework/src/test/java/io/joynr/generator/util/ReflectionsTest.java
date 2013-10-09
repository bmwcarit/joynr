package io.joynr.generator.util;

/*
 * #%L
 * io.joynr.tools.generator:generator-framework
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

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import io.joynr.generator.IGeneratorWithHeaders;
import io.joynr.generator.IJoynrGenerator;
import io.outofscope.OutOfScopeJoynGenerator;

import java.util.Arrays;
import java.util.Collection;
import java.util.Set;

import org.hamcrest.BaseMatcher;
import org.hamcrest.Description;
import org.hamcrest.Matcher;
import org.junit.Test;
import org.reflections.Reflections;

public class ReflectionsTest {

    @Test
    public void testReflection() throws Exception {
        Reflections reflections = new Reflections("io.joynr.generator");
        Set<Class<? extends IJoynrGenerator>> subTypes = reflections.getSubTypesOf(IJoynrGenerator.class);
        assertTrue(subTypes.contains(TestJoynrGenerator.class));
        assertTrue(subTypes.contains(IGeneratorWithHeaders.class));
        assertFalse(subTypes.contains(OutOfScopeJoynGenerator.class));
        Reflections reflections2 = new Reflections("");
        subTypes = reflections2.getSubTypesOf(IJoynrGenerator.class);
        assertTrue(subTypes.contains(TestJoynrGenerator.class));
        assertTrue(subTypes.contains(IGeneratorWithHeaders.class));
        assertTrue(subTypes.contains(OutOfScopeJoynGenerator.class));
    }

    public <T> Matcher<Set<? super T>> are(final T... ts) {
        final Collection<?> c1 = Arrays.asList(ts);
        return new BaseMatcher<Set<? super T>>() {
            public boolean matches(Object o) {
                Collection<?> c2 = (Collection<?>) o;
                return c1.containsAll(c2) && c2.containsAll(c1);
            }

            public void describeTo(Description description) {
                description.appendText("elements: ");
                description.appendValueList("(", ",", ")", ts);
            }
        };
    }

}
