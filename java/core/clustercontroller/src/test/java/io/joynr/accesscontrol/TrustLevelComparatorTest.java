package io.joynr.accesscontrol;

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

import static org.junit.Assert.assertEquals;
import joynr.infrastructure.DacTypes.TrustLevel;

import org.junit.Test;

public class TrustLevelComparatorTest {

    @Test
    public void testTrustLevelCompare() {
        int compareResult = TrustLevelComparator.compare(TrustLevel.HIGH, TrustLevel.MID);
        int expectedCompareResult = 1;
        assertEquals("HIGH should be greater than anything else", expectedCompareResult, compareResult);

        compareResult = TrustLevelComparator.compare(TrustLevel.MID, TrustLevel.HIGH);
        expectedCompareResult = -1;
        assertEquals("MID should be less than HIGH", expectedCompareResult, compareResult);

        compareResult = TrustLevelComparator.compare(TrustLevel.LOW, TrustLevel.LOW);
        expectedCompareResult = 0;
        assertEquals("two trust levels values should be the same", expectedCompareResult, compareResult);
    }
}
