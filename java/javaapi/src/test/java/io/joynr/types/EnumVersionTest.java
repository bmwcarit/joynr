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

import joynr.tests.EnumInsideInterface;
import joynr.tests.EnumInsideInterfaceWithoutVersion;
import joynr.tests.testTypes.TestEnum;
import joynr.types.TestTypesWithoutVersion.EnumInsideTypeCollectionWithoutVersion;

@RunWith(MockitoJUnitRunner.class)
public class EnumVersionTest {
    @Test
    public void versionIsSetInEnumInsideInterface() {
        int expectedMajorVersion = 47;
        int expectedMinorVersion = 11;
        Assert.assertEquals(expectedMajorVersion, EnumInsideInterface.MAJOR_VERSION);
        Assert.assertEquals(expectedMinorVersion, EnumInsideInterface.MINOR_VERSION);
    }

    @Test
    public void defaultVersionIsSetInEnumInsideInterfaceWithoutVersion() {
        int expectedMajorVersion = 0;
        int expectedMinorVersion = 0;
        Assert.assertEquals(expectedMajorVersion, EnumInsideInterfaceWithoutVersion.MAJOR_VERSION);
        Assert.assertEquals(expectedMinorVersion, EnumInsideInterfaceWithoutVersion.MINOR_VERSION);
    }

    @Test
    public void versionIsSetInEnumInsideTypeCollection() {
        int expectedMajorVersion = 48;
        int expectedMinorVersion = 12;
        Assert.assertEquals(expectedMajorVersion, TestEnum.MAJOR_VERSION);
        Assert.assertEquals(expectedMinorVersion, TestEnum.MINOR_VERSION);
    }

    @Test
    public void defaultVersionIsSetInEnumInsideTypeCollectionWithoutVersion() {
        int expectedMajorVersion = 0;
        int expectedMinorVersion = 0;
        Assert.assertEquals(expectedMajorVersion, EnumInsideTypeCollectionWithoutVersion.MAJOR_VERSION);
        Assert.assertEquals(expectedMinorVersion, EnumInsideTypeCollectionWithoutVersion.MINOR_VERSION);
    }
}
