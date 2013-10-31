package io.joynr.messaging;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

import java.util.Map;

import joynr.infrastructure.ChannelUrlDirectoryProxy;
import joynr.types.ChannelUrlInformation;

import com.google.inject.AbstractModule;
import com.google.inject.Provides;

public class TestChannelUrlModule extends AbstractModule {

    ChannelUrlDirectoryProxy client;

    public TestChannelUrlModule(Map<String, ChannelUrlInformation> entries) {
        client = new TestChannelUrlClientImpl(entries);
    }

    @Override
    protected void configure() {
    }

    @Provides
    ChannelUrlDirectoryProxy provideChannelUrlClient() {
        return client;
    }

}
