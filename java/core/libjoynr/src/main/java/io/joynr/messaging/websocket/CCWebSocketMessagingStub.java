package io.joynr.messaging.websocket;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

import com.fasterxml.jackson.databind.ObjectMapper;
import org.eclipse.jetty.util.FuturePromise;
import org.eclipse.jetty.websocket.api.Session;

/**
 *
 * Messaging stub used on ClusterController side. It gets an already initialized session from the CCWebSocketMessagingSkeleton
 *
 */
public class CCWebSocketMessagingStub extends WebSocketMessagingStub {

    public CCWebSocketMessagingStub(Session session, ObjectMapper objectMapper) {
        super(objectMapper);
        sessionFuture = new FuturePromise<>(session);
    }

    @Override
    protected void initConnection() {
        //nothing to do here, connection is already initialized by CCWebSocketMessagingSkeleton
    }
}
