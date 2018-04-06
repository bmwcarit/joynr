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

var mod = require("module");

var wscppSpy = jasmine.createSpy("wscppSyp");
var overriddenRequire = mod.prototype.require;
mod.prototype.require = function(md) {
    if (md.endsWith("wscpp")) {
        return wscppSpy;
    }

    return overriddenRequire.apply(this, arguments);
};

// req path starting at: node-run-unit-tests
var WebsocketNode = req("../../main/js/global/WebSocketNode");

describe("websocket node", function() {
    var websocketNode;
    var remoteUrl = "url";
    var keychain = {
        ownerId: "ownerId"
    };
    var keychainWithCerts = {
        tlsCert: "tlsCert",
        tlsKey: "tlsKey",
        tlsCa: "tlsCa",
        ownerId: "ownerID"
    };

    it("calls the wscpp constructor with certs for unencrypted Tls communication", function() {
        var useUnencryptedTls = true;
        websocketNode = new WebsocketNode(remoteUrl, keychainWithCerts, useUnencryptedTls);

        expect(wscppSpy).toHaveBeenCalledWith(remoteUrl, {
            cert: keychainWithCerts.tlsCert,
            key: keychainWithCerts.tlsKey,
            ca: keychainWithCerts.tlsCa,
            rejectUnauthorized: true,
            useUnencryptedTls: useUnencryptedTls
        });
    });

    it("calls the wscpp constructor with certs  for encrypted Tls communication", function() {
        var useUnencryptedTls = false;
        websocketNode = new WebsocketNode(remoteUrl, keychainWithCerts, useUnencryptedTls);

        expect(wscppSpy).toHaveBeenCalledWith(remoteUrl, {
            cert: keychainWithCerts.tlsCert,
            key: keychainWithCerts.tlsKey,
            ca: keychainWithCerts.tlsCa,
            rejectUnauthorized: true,
            useUnencryptedTls: useUnencryptedTls
        });
    });
});
