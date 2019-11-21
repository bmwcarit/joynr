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

/**
 * Adapts the nodejs websocket lib WebSocket-Node to the WebSocket client API.
 * See: http://dev.w3.org/html5/websockets/#the-websocket-interface
 *
 */
import ws from "ws";

interface KeyChain {
    tlsCert: any;
    tlsKey: any;
    tlsCa: any;
    checkServerIdentity: any;
}

class WebSocketNode extends ws {
    public static CONNECTING = ws.CONNECTING;
    public static OPEN = ws.OPEN;
    public static CLOSING = ws.CLOSING;
    public static CLOSED = ws.CLOSED;

    public constructor(remoteUrl: string, keychain: KeyChain, useUnencryptedTls: boolean) {
        const clientOptions: ws.ClientOptions = {};
        if (keychain) {
            clientOptions.cert = keychain.tlsCert;
            clientOptions.key = keychain.tlsKey;
            clientOptions.ca = keychain.tlsCa;
            clientOptions.rejectUnauthorized = true;
            clientOptions.checkServerIdentity = keychain.checkServerIdentity;
        }

        if (useUnencryptedTls) {
            clientOptions.ciphers = "eNULL:@SECLEVEL=0";
        }

        super(remoteUrl, clientOptions);
    }
}

export = WebSocketNode;
