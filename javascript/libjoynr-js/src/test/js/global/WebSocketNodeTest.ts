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

const websocketConstructor = jest.fn();
// need absolute path because it's not the main node_modules folder of the tests
jest.doMock("../../../main/js/node_modules/ws", () => websocketConstructor);

import WebSocketNode = require("../../../main/js/global/WebSocketNode");

function checkServerIdentity() {}

describe("websocket node", () => {
    let websocketNode: WebSocketNode;
    const remoteUrl = "url";
    const keychainWithCerts = {
        tlsCert: "tlsCert",
        tlsKey: "tlsKey",
        tlsCa: "tlsCa",
        ownerId: "ownerID",
        checkServerIdentity
    };

    beforeEach(() => {
        jest.clearAllMocks();
    });

    it("calls the wscpp constructor with certs for unencrypted Tls communication", () => {
        const useUnencryptedTls = true;
        websocketNode = new WebSocketNode(remoteUrl, keychainWithCerts, useUnencryptedTls);

        expect(websocketNode).toBeDefined();
        expect(websocketConstructor).toHaveBeenCalledWith(remoteUrl, {
            cert: keychainWithCerts.tlsCert,
            key: keychainWithCerts.tlsKey,
            ca: keychainWithCerts.tlsCa,
            rejectUnauthorized: true,
            ciphers: "eNULL:@SECLEVEL=0",
            checkServerIdentity
        });
    });

    it("calls the wscpp constructor with certs for encrypted Tls communication", () => {
        const useUnencryptedTls = false;
        websocketNode = new WebSocketNode(remoteUrl, keychainWithCerts, useUnencryptedTls);

        expect(websocketNode).toBeDefined();
        expect(websocketConstructor).toHaveBeenCalledWith(remoteUrl, {
            cert: keychainWithCerts.tlsCert,
            key: keychainWithCerts.tlsKey,
            ca: keychainWithCerts.tlsCa,
            rejectUnauthorized: true,
            checkServerIdentity
        });
    });
});
