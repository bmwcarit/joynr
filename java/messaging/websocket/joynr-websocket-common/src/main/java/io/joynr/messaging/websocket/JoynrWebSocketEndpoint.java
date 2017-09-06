package io.joynr.messaging.websocket;

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

import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.TimeUnit;

import io.joynr.messaging.FailureAction;
import io.joynr.messaging.SuccessAction;
import joynr.system.RoutingTypes.Address;

public interface JoynrWebSocketEndpoint {

    public static final Charset CHARSET = StandardCharsets.UTF_8;

    public void start();

    public void setMessageListener(IWebSocketMessagingSkeleton messaging);

    public void shutdown();

    public void writeBytes(Address to,
                           byte[] message,
                           long timeout,
                           TimeUnit unit,
                           SuccessAction successAction,
                           FailureAction failureAction);

    public void reconnect();
}
