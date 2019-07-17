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
import JoynrMessage from "../JoynrMessage";

import * as Typing from "../../util/Typing";
import LoggingManager from "../../system/LoggingManager";
import SharedWebSocket = require("./SharedWebSocket");
const log = LoggingManager.getLogger("joynr.messaging.websocket.WebSocketMessagingSkeleton");

class WebSocketMessagingSkeleton {
    private listener?: Function;
    private sharedWebSocket: SharedWebSocket;
    /**
     * @constructor WebSocketMessagingSkeleton
     * @param settings
     * @param settings.sharedWebSocket
     * @param settings.mainTransport
     */
    public constructor(settings: { sharedWebSocket: SharedWebSocket; mainTransport: boolean }) {
        this.sharedWebSocket = settings.sharedWebSocket;

        settings.sharedWebSocket.onmessage = (joynrMessage: JoynrMessage) => {
            log.debug(`<<< INCOMING <<< message with ID ${joynrMessage.msgId}`);
            if (this.listener !== undefined) {
                if (joynrMessage.type === JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST && settings.mainTransport) {
                    joynrMessage.isReceivedFromGlobal = true;
                }
                this.listener(joynrMessage);
            }
        };
    }

    /**
     * Registers the listener function
     *
     * @param listener a listener function that should be added and should receive messages
     */
    public registerListener(listener: Function): void {
        Typing.checkProperty(listener, "Function", "listenerToAdd");

        this.listener = listener;
    }

    /**
     * Unregisters the listener function
     *
     * @param _listener the listener function that should re removed and shouldn't receive
     *            messages any more
     */
    public unregisterListener(_listener: Function): void {
        this.listener = undefined;
    }

    /* */
    public shutdown(): void {
        this.sharedWebSocket.close();
    }
}

export = WebSocketMessagingSkeleton;
