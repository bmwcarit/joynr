package io.joynr.runtime;

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

import com.google.inject.Inject;
import com.google.inject.name.Named;

public abstract class AbstractJoynrApplication extends JoynrApplication {
    public static final String PROPERTY_JOYNR_DOMAIN_LOCAL = "joynr.domain.local";
    @Inject(optional = true)
    @Named(PROPERTY_JOYNR_DOMAIN_LOCAL)
    protected String localDomain = "localdomain";

    @Inject
    protected JoynrRuntime runtime;

    @Override
    public void shutdown() {
        if (runtime != null) {
            runtime.shutdown(true);
        }
    }
}
