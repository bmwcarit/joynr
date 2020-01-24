/*
 * #%L
 * joynr::java::messaging::bounceproxy::controlled-bounceproxy
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
package io.joynr.messaging.bounceproxy.runtime;

import org.junit.Assert;
import org.junit.Test;

import io.joynr.exceptions.JoynrRuntimeException;

public class BounceProxySystemPropertyLoaderTest {

    @Test
    public void testReplacementOnString() {
        String value = "nothingToReplace";
        String result = BounceProxySystemPropertyLoader.replaceVariableBySystemProperty(value).get();
        Assert.assertEquals(value, result);
    }

    @Test
    public void testReplacementForSingleVariable() {
        String value = "${testproperty}";
        System.setProperty("testproperty", "correctreplacement");
        String result = BounceProxySystemPropertyLoader.replaceVariableBySystemProperty(value).get();
        Assert.assertEquals("correctreplacement", result);
        System.clearProperty("testproperty");
    }

    @Test
    public void testReplacementForSingleVariableWithAppendix() {
        String value = "${testproperty}.appendix";
        System.setProperty("testproperty", "correctreplacement");
        String result = BounceProxySystemPropertyLoader.replaceVariableBySystemProperty(value).get();
        Assert.assertEquals("correctreplacement.appendix", result);
        System.clearProperty("testproperty");
    }

    @Test
    public void testReplacementForSingleVariableWithPrefix() {
        String value = "prefix.${testproperty}";
        System.setProperty("testproperty", "correctreplacement");
        String result = BounceProxySystemPropertyLoader.replaceVariableBySystemProperty(value).get();
        Assert.assertEquals("prefix.correctreplacement", result);
        System.clearProperty("testproperty");
    }

    @Test
    public void testReplacementForSingleVariableWithPrefixAndAppendix() {
        String value = "prefix.${testproperty}.appendix";
        System.setProperty("testproperty", "correctreplacement");
        String result = BounceProxySystemPropertyLoader.replaceVariableBySystemProperty(value).get();
        Assert.assertEquals("prefix.correctreplacement.appendix", result);
        System.clearProperty("testproperty");
    }

    @Test
    public void testReplacementForTwoVariables() {
        String value = "${testproperty1}${testproperty2}";
        System.setProperty("testproperty1", "correctreplacement1");
        System.setProperty("testproperty2", "correctreplacement2");
        String result = BounceProxySystemPropertyLoader.replaceVariableBySystemProperty(value).get();
        Assert.assertEquals("correctreplacement1correctreplacement2", result);
        System.clearProperty("testproperty1");
        System.clearProperty("testproperty2");
    }

    @Test
    public void testReplacementForTwoVariablesWithPrefix() {
        String value = "prefix.${testproperty1}${testproperty2}";
        System.setProperty("testproperty1", "correctreplacement1");
        System.setProperty("testproperty2", "correctreplacement2");
        String result = BounceProxySystemPropertyLoader.replaceVariableBySystemProperty(value).get();
        Assert.assertEquals("prefix.correctreplacement1correctreplacement2", result);
        System.clearProperty("testproperty1");
        System.clearProperty("testproperty2");
    }

    @Test
    public void testReplacementForTwoVariablesWithAppendix() {
        String value = "${testproperty1}${testproperty2}.appendix";
        System.setProperty("testproperty1", "correctreplacement1");
        System.setProperty("testproperty2", "correctreplacement2");
        String result = BounceProxySystemPropertyLoader.replaceVariableBySystemProperty(value).get();
        Assert.assertEquals("correctreplacement1correctreplacement2.appendix", result);
        System.clearProperty("testproperty1");
        System.clearProperty("testproperty2");
    }

    @Test
    public void testReplacementForTwoVariablesWithInfix() {
        String value = "${testproperty1}.infix.${testproperty2}";
        System.setProperty("testproperty1", "correctreplacement1");
        System.setProperty("testproperty2", "correctreplacement2");
        String result = BounceProxySystemPropertyLoader.replaceVariableBySystemProperty(value).get();
        Assert.assertEquals("correctreplacement1.infix.correctreplacement2", result);
        System.clearProperty("testproperty1");
        System.clearProperty("testproperty2");
    }

    @Test
    public void testReplacementForTwoVariablesWithPrefixAndAppendix() {
        String value = "prefix.${testproperty1}${testproperty2}.appendix";
        System.setProperty("testproperty1", "correctreplacement1");
        System.setProperty("testproperty2", "correctreplacement2");
        String result = BounceProxySystemPropertyLoader.replaceVariableBySystemProperty(value).get();
        Assert.assertEquals("prefix.correctreplacement1correctreplacement2.appendix", result);
        System.clearProperty("testproperty1");
        System.clearProperty("testproperty2");
    }

    @Test
    public void testReplacementForTwoVariablesWithPrefixAndAppendixAndInfix() {
        String value = "prefix.${testproperty1}.infix.${testproperty2}.appendix";
        System.setProperty("testproperty1", "correctreplacement1");
        System.setProperty("testproperty2", "correctreplacement2");
        String result = BounceProxySystemPropertyLoader.replaceVariableBySystemProperty(value).get();
        Assert.assertEquals("prefix.correctreplacement1.infix.correctreplacement2.appendix", result);
        System.clearProperty("testproperty1");
        System.clearProperty("testproperty2");
    }

    @Test
    public void testReplacementForSingleVariableWithUnsetSystemProperty() {

        try {
            String value = "${testproperty}";
            BounceProxySystemPropertyLoader.replaceVariableBySystemProperty(value);
            Assert.fail("Should throw a runtime exception");
        } catch (JoynrRuntimeException e) {

        }
    }
}
