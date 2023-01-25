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
package io.joynr.provider;

/**
 * Concrete implementation of the {@link AbstractDeferred} class which permits an array of
 * values to be passed in, for example if representing the result of a multi-out method call.
 */
public class MultiValueDeferred extends AbstractDeferred {

    /**
     * Deliberately passes the values straight through to the abstract deferred so that
     * they will be resolved as multiple values.
     *
     * @param values the result which resolves the Deferred.
     * @return <code>true</code> if the resolution was successful, <code>false</code> otherwise.
     */
    public synchronized boolean resolve(Object... values) {
        return super.resolve(values);
    }
}
