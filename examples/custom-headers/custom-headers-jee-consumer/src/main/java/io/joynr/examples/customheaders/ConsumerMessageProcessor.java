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
package io.joynr.examples.customheaders;

import java.util.HashMap;
import java.util.Map;
import java.util.UUID;

import javax.ejb.Stateless;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.messaging.JoynrMessageProcessor;
import joynr.ImmutableMessage;
import joynr.MutableMessage;

@Stateless
public class ConsumerMessageProcessor implements JoynrMessageProcessor {

    private static final Logger LOG = LoggerFactory.getLogger(ConsumerMessageProcessor.class);

    @Override
    public MutableMessage processOutgoing(MutableMessage joynrMessage) {
        Map<String, String> customHeaders = joynrMessage.getCustomHeaders();
        if (customHeaders == null) {
            customHeaders = new HashMap<>();
            joynrMessage.setCustomHeaders(customHeaders);
        }
        String processorCustomHeader = UUID.randomUUID().toString();
        customHeaders.put("processor-custom-header", processorCustomHeader);
        LOG.info("Set processor-custom-header to {}", processorCustomHeader);
        return joynrMessage;
    }

    @Override
    public ImmutableMessage processIncoming(ImmutableMessage joynrMessage) {
        LOG.info("Processing incoming message {}", joynrMessage);
        LOG.info("Custom headers are: {}", joynrMessage.getCustomHeaders());
        LOG.info("Message context is: {}", joynrMessage.getContext());
        return joynrMessage;
    }
}
