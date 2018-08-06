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
package io.joynr.proxy;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.messaging.MessagingPropertyKeys;

@Singleton
public class DefaultStatelessAsyncIdCalculatorImpl implements StatelessAsyncIdCalculator {

    private static final Logger logger = LoggerFactory.getLogger(DefaultStatelessAsyncIdCalculatorImpl.class);

    private final String channelId;

    @Inject
    public DefaultStatelessAsyncIdCalculatorImpl(@Named(MessagingPropertyKeys.CHANNELID) String channelId) {
        this.channelId = channelId;
    }

    @Override
    public String calculateParticipantId(String interfaceName, StatelessAsyncCallback statelessAsyncCallback) {
        return channelId + ":>:" + calculateStatelessCallbackId(interfaceName, statelessAsyncCallback);
    }

    @Override
    public String calculateStatelessCallbackId(String interfaceName, StatelessAsyncCallback statelessAsyncCallback) {
        return interfaceName + ":~:" + statelessAsyncCallback.getUseCaseName();
    }
}
