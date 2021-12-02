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

import joynr.tests.MapInsideInterface;
import joynr.tests.MapInsideInterfaceWithoutVersion;
import joynr.types.TestTypes.TStringKeyMap;
import joynr.types.TestTypesWithoutVersion.MapInsideTypeCollectionWithoutVersion;

@RunWith(MockitoJUnitRunner.class)
public class MapVersionTest {
    @Test
    public void versionIsSetInMapInsideInterface() {
        int expectedMajorVersion = 47;
        int expectedMinorVersion = 11;
        Assert.assertEquals(expectedMajorVersion, MapInsideInterface.MAJOR_VERSION);
        Assert.assertEquals(expectedMinorVersion, MapInsideInterface.MINOR_VERSION);
    }

    @Test
    public void defaultVersionIsSetInMapInsideInterfaceWithoutVersion() {
        int expectedMajorVersion = 0;
        int expectedMinorVersion = 0;
        Assert.assertEquals(expectedMajorVersion, MapInsideInterfaceWithoutVersion.MAJOR_VERSION);
        Assert.assertEquals(expectedMinorVersion, MapInsideInterfaceWithoutVersion.MINOR_VERSION);
    }

    @Test
    public void versionIsSetInMapInsideTypeCollection() {
        int expectedMajorVersion = 49;
        int expectedMinorVersion = 13;
        Assert.assertEquals(expectedMajorVersion, TStringKeyMap.MAJOR_VERSION);
        Assert.assertEquals(expectedMinorVersion, TStringKeyMap.MINOR_VERSION);
    }

    @Test
    public void defaultVersionIsSetInMapInsideTypeCollectionWithoutVersion() {
        int expectedMajorVersion = 0;
        int expectedMinorVersion = 0;
        Assert.assertEquals(expectedMajorVersion, MapInsideTypeCollectionWithoutVersion.MAJOR_VERSION);
        Assert.assertEquals(expectedMinorVersion, MapInsideTypeCollectionWithoutVersion.MINOR_VERSION);
    }
}
