/*-
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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
package io.joynr.tests.gracefulshutdown;

import jakarta.ejb.Stateless;
import jakarta.inject.Inject;

import io.joynr.jeeintegration.api.ServiceProvider;
import joynr.tests.graceful.shutdown.SecondLevelServiceSync;

@Stateless
@ServiceProvider(serviceInterface = SecondLevelServiceSync.class)
public class SecondLevelServiceBean implements SecondLevelServiceSync {

    @Inject
    private FailureLogBean failureLogBean;

    @Override
    public String transform(String inData) {
        return inData + " // " + System.currentTimeMillis();
    }

    @Override
    public void logFailure(String inData) {
        failureLogBean.add(inData);
    }
}
