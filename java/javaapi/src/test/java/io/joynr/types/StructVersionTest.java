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
package io.joynr.types;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.junit.MockitoJUnitRunner;

import joynr.tests.StructInsideInterface;
import joynr.tests.StructInsideInterfaceWithoutVersion;
import joynr.tests.testTypes.ComplexTestType;
import joynr.types.TestTypesWithoutVersion.StructInsideTypeCollectionWithoutVersion;

@RunWith(MockitoJUnitRunner.class)
public class StructVersionTest {
    @Test
    public void versionIsSetInStructInsideInterface() {
        int expectedMajorVersion = 47;
        int expectedMinorVersion = 11;
        Assert.assertEquals(expectedMajorVersion, StructInsideInterface.MAJOR_VERSION);
        Assert.assertEquals(expectedMinorVersion, StructInsideInterface.MINOR_VERSION);
    }

    @Test
    public void defaultVersionIsSetInStructInsideInterfaceWithoutVersion() {
        int expectedMajorVersion = 0;
        int expectedMinorVersion = 0;
        Assert.assertEquals(expectedMajorVersion, StructInsideInterfaceWithoutVersion.MAJOR_VERSION);
        Assert.assertEquals(expectedMinorVersion, StructInsideInterfaceWithoutVersion.MINOR_VERSION);
    }

    @Test
    public void versionIsSetInStructInsideTypeCollection() {
        int expectedMajorVersion = 48;
        int expectedMinorVersion = 12;
        Assert.assertEquals(expectedMajorVersion, ComplexTestType.MAJOR_VERSION);
        Assert.assertEquals(expectedMinorVersion, ComplexTestType.MINOR_VERSION);
    }

    @Test
    public void defaultVersionIsSetInStructInsideTypeCollectionWithoutVersion() {
        int expectedMajorVersion = 0;
        int expectedMinorVersion = 0;
        Assert.assertEquals(expectedMajorVersion, StructInsideTypeCollectionWithoutVersion.MAJOR_VERSION);
        Assert.assertEquals(expectedMinorVersion, StructInsideTypeCollectionWithoutVersion.MINOR_VERSION);
    }
}
