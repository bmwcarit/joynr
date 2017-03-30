package io.joynr.systemintegrationtest;

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
import io.joynr.provider.Promise;

import joynr.test.SystemIntegrationTestAbstractProvider;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class Provider extends SystemIntegrationTestAbstractProvider {
    private static final Logger logger = LoggerFactory.getLogger(Provider.class);

    public Provider() {
    }

    /*
     * add
     */
    @Override
    public Promise<AddDeferred> add(Integer addendA, Integer addendB) {
        logger.info("Provider.add(" + addendA + ", " + addendB + ") called");
        AddDeferred deferred = new AddDeferred();

        deferred.resolve(addendA + addendB);
        return new Promise<AddDeferred>(deferred);
    }
}
