/*-
 * #%L
 * %%
 * Copyright (C) 2011 - 2018 BMW Car IT GmbH
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
package io.joynr.jeeintegration.servicelocator;

import jakarta.ejb.Stateless;

import io.joynr.jeeintegration.api.CallbackHandler;
import joynr.jeeintegration.servicelocator.MyServiceStatelessAsyncCallback;

@Stateless
@CallbackHandler
public class MyServiceCallbackHandler implements MyServiceStatelessAsyncCallback {

    public static final String USE_CASE = "useCase";

    @Override
    public String getUseCase() {
        return USE_CASE;
    }
}
