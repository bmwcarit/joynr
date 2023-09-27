/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
package io.joynr.test.interlanguage;

import com.google.inject.Inject;

import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.util.ObjectMapper;

public class IltDummyApplication extends AbstractJoynrApplication {

    @Inject
    private ObjectMapper objectMapper;

    @Override
    public void shutdown() {
        runtime.shutdown(true);
    }

    @Override
    public void run() {

    }

    public ObjectMapper getObjectMapper() {
        return objectMapper;
    }

    public String getLocalDomain() {
        return localDomain;
    }

    public JoynrRuntime getRuntime() {
        return runtime;
    }
}
