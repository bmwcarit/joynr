package io.joynr.tests.gracefulshutdown;

/*-
 * #%L
 * %%
 * Copyright (C) 2011 - 2025 BMW Car IT GmbH
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

import jakarta.annotation.PostConstruct;
import jakarta.ejb.Singleton;
import jakarta.inject.Inject;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.jeeintegration.api.ServiceLocator;
import joynr.tests.graceful.shutdown.SecondLevelServiceSync;

@Singleton
public class SecondLevelServiceClient {

    private static final Logger logger = LoggerFactory.getLogger(SecondLevelServiceClient.class);

    @Inject
    private ServiceLocator serviceLocator;

    private SecondLevelServiceSync secondLevelServiceSync;

    @PostConstruct
    public void initialise() {
        secondLevelServiceSync = serviceLocator.get(SecondLevelServiceSync.class,
                                                    "io.joynr.tests.gracefulshutdown.jee.secondlevel.provider");
    }

    public SecondLevelServiceSync get() {
        return secondLevelServiceSync;
    }
}
