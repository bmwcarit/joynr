/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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

import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ExpectedException;
import org.junit.runner.RunWith;
import org.mockito.junit.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class StructSetterTest {
    @Rule
    public ExpectedException expectedException = ExpectedException.none();

    // joynr.types.TestTypes.TStruct is generated without extra parameters

    @Test
    public void setterThrowsOnNullValue() {
        joynr.types.TestTypes.TStruct tStruct = new joynr.types.TestTypes.TStruct();

        expectedException.expect(IllegalArgumentException.class);
        expectedException.expectMessage("setting tString to null is not allowed");

        tStruct.setTString(null);
    }

    // joynr.types.TestTypes2.TStruct is generated with optional parameter
    // ignoreInvalidNullClassMembers true

    @Test
    public void setterDoesNotThrowOnNullValue() {
        joynr.types.TestTypes2.TStruct tStruct = new joynr.types.TestTypes2.TStruct();

        tStruct.setTString(null);
    }
}
