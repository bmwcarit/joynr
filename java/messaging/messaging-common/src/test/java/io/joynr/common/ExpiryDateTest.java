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

import static io.joynr.common.ExpiryDate.fromAbsolute;
import static io.joynr.common.ExpiryDate.fromRelativeTtl;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import org.junit.Before;
import org.junit.Test;

public class ExpiryDateTest {

    private static final long ONE_SECOND = 1000L;
    private static final long TWO_SECONDS = 2000L;
    private static final long LESS_THAN_SECOND = 100L;
    private static final long MAX_DIFF = 50L;
    private static final long MAX_JS_INT = 9007199254740991L;

    private long currentTime;

    @Before
    public void setUp() {
        currentTime = System.currentTimeMillis();
    }

    @Test
    public void fromRelativeTtlHappyPass() {
        final long relativeTtl = ONE_SECOND;
        final ExpiryDate expiryDate = fromRelativeTtl(relativeTtl);

        checkExactTtlValue(relativeTtl, expiryDate);
        checkValueInRange(currentTime, relativeTtl, expiryDate.getValue());
    }

    @Test
    public void fromRelativeTtlHappyPassWithPause() {
        final long relativeTtl = TWO_SECONDS;
        // pause is used to simulate not immediate processing
        pause();
        final ExpiryDate expiryDate = fromRelativeTtl(relativeTtl);

        checkExactTtlValue(relativeTtl, expiryDate);
        checkValueInRange(currentTime, relativeTtl, expiryDate.getValue());
    }

    @Test
    public void fromRelativeTtlIfMaxLongValueExceeded() {
        // simulate case when relative time to live + current time slightly exceeds max long value
        final long relativeTtl = Long.MAX_VALUE - currentTime + LESS_THAN_SECOND;
        final ExpiryDate expiryDate = fromRelativeTtl(relativeTtl);

        checkExactTtlValue(relativeTtl, expiryDate);
        checkValueIsMax(expiryDate.getValue());
    }

    @Test
    public void fromRelativeTtlIfMaxLongValueExceededWithPause() {
        // simulate case when relative time to live + current time slightly exceeds max long value
        final long relativeTtl = Long.MAX_VALUE - currentTime + LESS_THAN_SECOND;
        // pause is used to simulate not immediate processing
        pause();
        final ExpiryDate expiryDate = fromRelativeTtl(relativeTtl);

        checkExactTtlValue(relativeTtl, expiryDate);
        checkValueIsMax(expiryDate.getValue());
    }

    @Test
    public void fromRelativeTtlIfMaxJSIntValueExceeded() {
        // simulate case when relative time to live + current time slightly exceeds max JS int value
        final long relativeTtl = MAX_JS_INT - currentTime + LESS_THAN_SECOND;
        final ExpiryDate expiryDate = fromRelativeTtl(relativeTtl);

        checkExactTtlValue(relativeTtl, expiryDate);
        checkValueIsMax(expiryDate.getValue());
    }

    @Test
    public void fromRelativeTtlIfMaxJSIntValueExceededWithPause() {
        // simulate case when relative time to live + current time slightly exceeds max JS int value
        final long relativeTtl = MAX_JS_INT - currentTime + LESS_THAN_SECOND;
        // pause is used to simulate not immediate processing
        pause();
        final ExpiryDate expiryDate = fromRelativeTtl(relativeTtl);

        checkExactTtlValue(relativeTtl, expiryDate);
        checkValueIsMax(expiryDate.getValue());
    }

    @Test
    public void fromAbsoluteHappyPass() {
        final long relativeTtl = TWO_SECONDS;
        final long absolute = currentTime + relativeTtl;
        final ExpiryDate expiryDate = fromAbsolute(absolute);

        checkTtlValueInRange(relativeTtl, expiryDate);
        checkValueInRange(currentTime, relativeTtl, expiryDate.getValue());
    }

    @Test
    public void fromAbsoluteHappyPassWithPause() {
        final long relativeTtl = ONE_SECOND;
        final long absolute = currentTime + relativeTtl;
        // pause is used to simulate not immediate processing
        pause();
        final ExpiryDate expiryDate = fromAbsolute(absolute);

        checkTtlValueInRange(relativeTtl, expiryDate);
        checkValueInRange(currentTime, relativeTtl, expiryDate.getValue());
    }

    @Test
    public void fromAbsoluteIfMinLongValueExceeded() {
        // simulate case when absolute time - current time slightly less than min long value
        final long absolute = Long.MIN_VALUE + currentTime - LESS_THAN_SECOND;
        final ExpiryDate expiryDate = fromAbsolute(absolute);

        checkTtlIsMin(expiryDate);
        checkExactValue(absolute, expiryDate);
    }

    @Test
    public void fromAbsoluteIfMinLongValueExceededWithPause() {
        // simulate case when absolute time - current time slightly less than min long value
        final long absolute = Long.MIN_VALUE + currentTime - LESS_THAN_SECOND;
        // pause is used to simulate not immediate processing
        pause();
        final ExpiryDate expiryDate = fromAbsolute(absolute);

        checkTtlIsMin(expiryDate);
        checkExactValue(absolute, expiryDate);
    }

    @Test
    public void toStringToContainValueAndTtl() {
        checkToStringValue(fromRelativeTtl(ONE_SECOND));
        checkToStringValue(fromAbsolute(currentTime + TWO_SECONDS));
    }

    private void checkToStringValue(final ExpiryDate expiryDate) {
        assertNotNull(expiryDate);
        final String toStringValue = expiryDate.toString();
        assertNotNull(toStringValue);
        assertTrue(toStringValue.contains(String.valueOf(expiryDate.getRelativeTtl())));
        assertTrue(toStringValue.contains(String.valueOf(expiryDate.getValue())));
    }

    private void pause() {
        try {
            Thread.sleep(25L);
        } catch (final InterruptedException exception) {
            fail("Unexpected exception: " + exception.getMessage());
            throw new RuntimeException(exception);
        }
    }

    private void checkValueInRange(final long expectedTime, final long expectedRelativeTtl, final long actualValue) {
        final long expectedValue = expectedTime + expectedRelativeTtl;
        checkValueInRange(expectedValue, actualValue);
    }

    private void checkValueInRange(final long expectedValue, final long actualValue) {
        assertTrue(actualValue >= expectedValue);
        assertTrue((actualValue - expectedValue) <= MAX_DIFF);
    }

    private void checkExactValue(final long expectedValue, final ExpiryDate expiryDate) {
        checkExactValue(expectedValue, expiryDate.getValue());
    }

    private void checkExactValue(final long expectedValue, final long actualValue) {
        assertEquals(expectedValue, actualValue);
    }

    private void checkExactTtlValue(final long expectedRelativeTtl, final ExpiryDate expiryDate) {
        assertNotNull(expiryDate);
        assertEquals(expectedRelativeTtl, expiryDate.getRelativeTtl());
    }

    private void checkTtlValueInRange(final long expectedRelativeTtl, final ExpiryDate expiryDate) {
        assertNotNull(expiryDate);
        final long actualValue = expiryDate.getRelativeTtl();
        assertTrue(actualValue <= expectedRelativeTtl);
        assertTrue((expectedRelativeTtl - actualValue) <= MAX_DIFF);
    }

    private void checkValueIsMax(final long actualValue) {
        assertEquals(MAX_JS_INT, actualValue);
    }

    private void checkTtlIsMin(final ExpiryDate expiryDate) {
        checkExactTtlValue(Long.MIN_VALUE, expiryDate);
    }
}
