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

import WebMessagingStubFactory from "../../../../../main/js/joynr/messaging/webmessaging/WebMessagingStubFactory";
import * as JSONSerializer from "../../../../../main/js/joynr/util/JSONSerializer";

describe("libjoynr-js.joynr.messaging.webmessaging.WebMessagingStubFactory", () => {
    let webMessagingStubFactory: WebMessagingStubFactory;
    let window: any;
    let origin: any;
    let webMessagingAddress: any;
    let joynrMessage: any;

    beforeEach(done => {
        webMessagingStubFactory = new WebMessagingStubFactory();

        window = {};
        window.postMessage = jest.fn();

        origin = "origin";
        webMessagingAddress = {
            getWindow: jest.fn(),
            getOrigin: jest.fn()
        };
        webMessagingAddress.getWindow.mockReturnValue(window);
        webMessagingAddress.getOrigin.mockReturnValue(origin);
        joynrMessage = {};
        done();
    });

    it("is instantiable and of correct type", () => {
        expect(WebMessagingStubFactory).toBeDefined();
        expect(typeof WebMessagingStubFactory === "function").toBeTruthy();
        expect(webMessagingStubFactory).toBeDefined();
        expect(webMessagingStubFactory.build).toBeDefined();
        expect(typeof webMessagingStubFactory.build === "function").toBeTruthy();
    });

    it("creates a messaging stub and uses it correctly", () => {
        const webMessagingStub = webMessagingStubFactory.build(webMessagingAddress);
        expect(webMessagingAddress.getWindow).toHaveBeenCalledWith();
        expect(webMessagingAddress.getOrigin).toHaveBeenCalledWith();

        const param = {
            message: joynrMessage
        };
        webMessagingStub.transmit(param);
        expect(window.postMessage).toHaveBeenCalledWith(JSON.parse(JSONSerializer.stringify(param)), origin);
    });
});
