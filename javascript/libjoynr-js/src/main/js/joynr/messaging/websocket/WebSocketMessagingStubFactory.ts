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
import * as WebSocketAddress from "../../../generated/joynr/system/RoutingTypes/WebSocketAddress";
import WebSocketMessagingStub from "./WebSocketMessagingStub";
import SharedWebSocket = require("./SharedWebSocket");

class WebSocketMessagingStubFactory {
    private webSocketMessagingStub: WebSocketMessagingStub;
    /**
     * @constructor
     * @param settings
     * @param settings.sharedWebSocket to the websocket server
     * @param settings.address of the websocket for the websocket server
     */
    public constructor(settings: { sharedWebSocket: SharedWebSocket; address: WebSocketAddress }) {
        this.webSocketMessagingStub = new WebSocketMessagingStub({
            sharedWebSocket: settings.sharedWebSocket
        });
    }

    public build(_address: WebSocketAddress): WebSocketMessagingStub {
        return this.webSocketMessagingStub;
    }
}

export = WebSocketMessagingStubFactory;
