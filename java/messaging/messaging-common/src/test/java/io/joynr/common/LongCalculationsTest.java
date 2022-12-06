/*
 * #%L
 * %%
 * Copyright (C) 2022 BMW Car IT GmbH
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
package io.joynr.common;

import static org.junit.Assert.assertEquals;

import org.junit.Test;

public class LongCalculationsTest {

    @Test
    public void addAndLimitHappyPass() {
        assertEquals(30, LongCalculations.addAndLimit(10, 20));
    }

    @Test
    public void addAndLimitReturnMaxAllowedValueIfOverflowOccurs() {
        assertEquals(LongCalculations.MAX_JS_INT, LongCalculations.addAndLimit(Long.MAX_VALUE - 10, 20));
    }

    @Test
    public void addAndLimitReturnMaxAllowedValueIfSumExceedsIt() {
        assertEquals(LongCalculations.MAX_JS_INT, LongCalculations.addAndLimit(LongCalculations.MAX_JS_INT - 10, 20));
    }

    @Test
    public void subtractHappyPass() {
        assertEquals(10, LongCalculations.subtract(30, 20));
    }

    @Test
    public void subtractReturnsMinAllowedValueIffOverflowOccurs() {
        assertEquals(Long.MIN_VALUE, LongCalculations.subtract(Long.MIN_VALUE + 10, 20));
    }
}
