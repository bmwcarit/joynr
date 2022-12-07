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

public class LongCalculations {

    //JavaScript max safe integer (2^53-1)
    public static final long MAX_JS_INT = 9007199254740991L;

    private static final LongToLongBiFunction ADD = (x, y) -> {
        try {
            return Math.addExact(x, y);
        } catch (final ArithmeticException exception) {
            return MAX_JS_INT;
        }
    };

    private static final LongToLongFunction LIMIT_MAX = x -> Math.min(x, MAX_JS_INT);

    private static final LongToLongBiFunction ADD_AND_LIMIT = ADD.andThen(LIMIT_MAX);

    private static final LongToLongBiFunction SUBTRACT = (x, y) -> {
        try {
            return Math.subtractExact(x, y);
        } catch (final ArithmeticException exception) {
            return Long.MIN_VALUE;
        }
    };

    /**
     * Adds first argument to second.
     * Result is limited to JavaScript max safe integer value.
     * @param first  first argument of sum
     * @param second second argument of sum
     * @return result of sum
     */
    public static long addAndLimit(final long first, final long second) {
        return ADD_AND_LIMIT.apply(first, second);
    }

    /**
     * Subtracts second argument from first
     * @param first  first argument of subtraction
     * @param second second argument of subtraction
     * @return result of subtraction
     */
    public static long subtract(final long first, final long second) {
        return SUBTRACT.apply(first, second);
    }
}
