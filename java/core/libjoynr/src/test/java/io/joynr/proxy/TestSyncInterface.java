/*
 * #%L
 * %%
 * Copyright (C) 2023 BMW Car IT GmbH
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
package io.joynr.proxy;

import io.joynr.Sync;
import io.joynr.dispatcher.rpc.MultiReturnValuesContainer;
import joynr.exceptions.ApplicationException;

@Sync
public interface TestSyncInterface {
    void methodWithoutParameters();

    void methodWithoutParametersWithModelledErrors() throws ApplicationException;

    Integer methodWithParameters(final Integer a, final Integer b);

    TestMultiResult methodReturningMultiValues();

    class TestMultiResult implements MultiReturnValuesContainer {

        public static final String VALUE_A = "[VALUE_A]";
        public static final String VALUE_B = "[VALUE_B]";

        private Object[] values;

        public TestMultiResult() {
            this.values = new Object[]{ VALUE_A, VALUE_B };
        }

        @Override
        public Object[] getValues() {
            return this.values;
        }

        public void setValues(final Object[] values) {
            this.values = values;
        }
    }
}