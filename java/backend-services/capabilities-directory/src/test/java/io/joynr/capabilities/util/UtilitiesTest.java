/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
package io.joynr.capabilities.util;

import static org.junit.Assert.assertEquals;

import org.junit.Test;

import io.joynr.capabilities.directory.util.Utilities;
import io.joynr.capabilities.directory.util.Utilities.ValidateGBIDsEnum;

public class UtilitiesTest {
    private final String validGcdGbId = "validGcdGbId";
    private final String[] validGbids = { validGcdGbId };

    @Test(expected = IllegalStateException.class)
    public void testFailureOnEmptyGcdGbId() {

        Utilities.validateGbids(validGbids, "");
    }

    @Test(expected = IllegalStateException.class)
    public void testFailureOnNullGcdGbId() {
        Utilities.validateGbids(validGbids, null);
    }

    @Test
    public void testReturnInvalidOnNullGbids() {
        assertEquals(ValidateGBIDsEnum.INVALID, Utilities.validateGbids(null, validGcdGbId));
    }

    @Test
    public void testReturnInvalidEmptyGbids() {
        final String[] invalidGbids = {};
        assertEquals(ValidateGBIDsEnum.INVALID, Utilities.validateGbids(invalidGbids, validGcdGbId));
    }

    @Test
    public void testReturnInvalidOnNullGbid() {
        final String[] invalidGbids = { null };
        assertEquals(ValidateGBIDsEnum.INVALID, Utilities.validateGbids(invalidGbids, validGcdGbId));
    }

    @Test
    public void testReturnInvalidOnEmptyGbid() {
        final String[] invalidGbids = { "" };
        assertEquals(ValidateGBIDsEnum.INVALID, Utilities.validateGbids(invalidGbids, validGcdGbId));
    }

    @Test
    public void testReturnUnknownOnUnknownGbid() {
        final String[] invalidGbids = { "wrong-gbid" };
        assertEquals(ValidateGBIDsEnum.UNKNOWN, Utilities.validateGbids(invalidGbids, validGcdGbId));
    }

    @Test
    public void testReturnOKOnValidGbid() {
        assertEquals(ValidateGBIDsEnum.OK, Utilities.validateGbids(validGbids, validGcdGbId));
    }
}
