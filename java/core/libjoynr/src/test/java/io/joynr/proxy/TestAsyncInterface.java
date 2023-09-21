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

import com.fasterxml.jackson.databind.JsonMappingException;
import io.joynr.Async;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcCallback;

@Async
public interface TestAsyncInterface {
    void someMethodWithoutAnnotations(final Integer a, final String b) throws JsonMappingException;

    void someMethodWithAnnotation(final Integer a,
                                  final String b,
                                  @JoynrRpcCallback(deserializationType = Void.class) final Callback<Void> callback)
            throws JsonMappingException;

    Future<Void> methodWithoutParameters(@JoynrRpcCallback(
            deserializationType = Void.class) final Callback<Void> callback);
}