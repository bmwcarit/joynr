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

import WebMessagingStub from "../../../../../main/js/joynr/messaging/webmessaging/WebMessagingStub";
import * as JSONSerializer from "../../../../../main/js/joynr/util/JSONSerializer";

describe("libjoynr-js.joynr.messaging.webmessaging.WebMessagingStub", () => {
    let window: any, origin: any, webMessagingStub: WebMessagingStub, joynrMessage: any;

    beforeEach(done => {
        window = {};
        window.postMessage = jest.fn();
        origin = "defaultOrigin";

        webMessagingStub = new WebMessagingStub({
            window,
            origin
        });

        joynrMessage = {};
        done();
    });

    it("is of correct type and has all members", () => {
        expect(WebMessagingStub).toBeDefined();
        expect(typeof WebMessagingStub === "function").toBeTruthy();
        expect(webMessagingStub).toBeDefined();
        expect(webMessagingStub.transmit).toBeDefined();
        expect(typeof webMessagingStub.transmit === "function").toBeTruthy();
    });

    it("calls window.postMessage correctly", () => {
        const param = {
            message: joynrMessage
        };
        webMessagingStub.transmit(param);
        expect(window.postMessage).toHaveBeenCalledWith(JSON.parse(JSONSerializer.stringify(param)), origin);
    });
});
