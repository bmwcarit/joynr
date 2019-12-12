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
package io.joynr.util;

import static org.junit.Assert.assertNotNull;

import java.util.HashMap;
import java.util.Map;

import org.junit.Assert;
import org.junit.Test;

import io.joynr.dispatcher.rpc.RequestStatus;

public class ReflectionUtilsTest {

    private void addSingleElementToFixture(Map<String, Class<?>[]> fixture,
                                           String expectedDatatypeName,
                                           Class<?> datatype) {
        fixture.put(expectedDatatypeName, new Class<?>[]{ datatype });
    }

    @Test
    public void testToDatatypeNames() {
        Map<String, Class<?>[]> fixture = new HashMap<>();
        addSingleElementToFixture(fixture, "Boolean", Boolean.class);
        addSingleElementToFixture(fixture, "Boolean[]", Boolean[].class);
        addSingleElementToFixture(fixture, "Byte", Byte.class);
        addSingleElementToFixture(fixture, "Byte[]", Byte[].class);
        addSingleElementToFixture(fixture, "Short", Short.class);
        addSingleElementToFixture(fixture, "Short[]", Short[].class);
        addSingleElementToFixture(fixture, "Integer", Integer.class);
        addSingleElementToFixture(fixture, "Integer[]", Integer[].class);
        addSingleElementToFixture(fixture, "Long", Long.class);
        addSingleElementToFixture(fixture, "Long[]", Long[].class);
        addSingleElementToFixture(fixture, "Float", Float.class);
        addSingleElementToFixture(fixture, "Float[]", Float[].class);
        addSingleElementToFixture(fixture, "Double", Double.class);
        addSingleElementToFixture(fixture, "Double[]", Double[].class);
        addSingleElementToFixture(fixture, "String", String.class);
        addSingleElementToFixture(fixture, "String[]", String[].class);
        addSingleElementToFixture(fixture, "io.joynr.dispatcher.rpc.RequestStatus", RequestStatus.class);
        addSingleElementToFixture(fixture, "io.joynr.dispatcher.rpc.RequestStatus[]", RequestStatus[].class);

        for (Map.Entry<String, Class<?>[]> entry : fixture.entrySet()) {
            String datatypeName = ReflectionUtils.toDatatypeNames(entry.getValue())[0];
            String key = entry.getKey();
            Assert.assertEquals(key, datatypeName);
        }
    }

    @Test
    public void testGetStaticMethodFromSuperInterfacesWithClassContainingSearchedMethod() throws NoSuchMethodException {
        assertNotNull(ReflectionUtils.getStaticMethodFromSuperInterfaces(TestClassContainingSearchedMethod.class,
                                                                         "oneMethodToRuleThemAll"));
    }

    @Test(expected = NoSuchMethodException.class)
    public void testGetStaticMethodFromSuperInterfacesWithClassNotContainingSearchedMethod() throws NoSuchMethodException {
        ReflectionUtils.getStaticMethodFromSuperInterfaces(TestClassNotContainingSearchedMethod.class,
                                                           "oneMethodToRuleThemAll");
    }

    @Test
    public void testGetStaticMethodFromSuperInterfacesWithParentClassContainingSearchedMethod() throws NoSuchMethodException {
        assertNotNull(ReflectionUtils.getStaticMethodFromSuperInterfaces(TestClassWithParentContainingSearchedMethod.class,
                                                                         "oneMethodToRuleThemAll"));
    }

    @Test(expected = NoSuchMethodException.class)
    public void testGetStaticMethodFromSuperInterfacesWithParentClassNotContainingSearchedMethod() throws NoSuchMethodException {
        ReflectionUtils.getStaticMethodFromSuperInterfaces(TestClassWithParentNotContainingSearchedMethod.class,
                                                           "oneMethodToRuleThemAll");
    }

    @Test
    public void testGetStaticMethodFromSuperInterfacesWithInterfaceContainingSearchedMethod() throws NoSuchMethodException {
        ReflectionUtils.getStaticMethodFromSuperInterfaces(TestInterfaceContainingSearchedMethod.class,
                                                           "oneMethodToRuleThemAll");
    }

    @Test(expected = NoSuchMethodException.class)
    public void testGetStaticMethodFromSuperInterfacesWithInterfaceNotContainingSearchedMethod() throws NoSuchMethodException {
        ReflectionUtils.getStaticMethodFromSuperInterfaces(TestInterfaceNotContainingSearchedMethod.class,
                                                           "oneMethodToRuleThemAll");
    }

    @Test
    public void testGetStaticMethodFromSuperInterfacesWithInterfaceExtendingInterfaceContainingSearchedMethod() throws NoSuchMethodException {
        ReflectionUtils.getStaticMethodFromSuperInterfaces(TestInterfaceExtendingInterfaceContainingSearchedMethod.class,
                                                           "oneMethodToRuleThemAll");
    }

    @Test(expected = NoSuchMethodException.class)
    public void testGetStaticMethodFromSuperInterfacesWithInterfaceExtendingInterfaceNotContainingSearchedMethod() throws NoSuchMethodException {
        ReflectionUtils.getStaticMethodFromSuperInterfaces(TestInterfaceExtendingInterfaceNotContainingSearchedMethod.class,
                                                           "oneMethodToRuleThemAll");
    }

    private class TestClassContainingSearchedMethod {
        public void oneMethodToRuleThemAll() {
        };
    }

    private class TestClassWithParentContainingSearchedMethod extends ParentClassContainingSearchedMethod {
    }

    private class TestClassNotContainingSearchedMethod {
    }

    private class TestClassWithParentNotContainingSearchedMethod extends ParentClassNotContainingSearchedMethod {
    }

    private class ParentClassContainingSearchedMethod {
        public void oneMethodToRuleThemAll() {
        };
    }

    private class ParentClassNotContainingSearchedMethod {
    }

    private interface TestInterfaceContainingSearchedMethod {
        public static void oneMethodToRuleThemAll() {
        };
    }

    private interface TestInterfaceNotContainingSearchedMethod {
    }

    private interface TestInterfaceExtendingInterfaceContainingSearchedMethod
            extends TestInterfaceContainingSearchedMethod {
    }

    private interface TestInterfaceExtendingInterfaceNotContainingSearchedMethod
            extends TestInterfaceNotContainingSearchedMethod {
    }
}
