/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
package itest.io.joynr.jeeintegration.exceptions;

import javax.ejb.Stateless;
import com.google.inject.Inject;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.jeeintegration.api.ServiceLocator;
import joynr.jeeintegration.servicelocator.MyServiceSync;

@Stateless
public class TestBean {

    private static final Logger logger = LoggerFactory.getLogger(TestBean.class);

    @Inject
    private ServiceLocator serviceLocator;

    public void testMethod() {
        serviceLocator.builder(MyServiceSync.class); // <- deliberate to trigger JoynrRuntimeException
    }
}
