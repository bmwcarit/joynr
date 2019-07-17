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

import InProcessMessagingStub from "../../../../../main/js/joynr/messaging/inprocess/InProcessMessagingStub";

describe("libjoynr-js.joynr.messaging.inprocess.InProcessMessagingStub", () => {
    let skeletonCallReturn: any,
        inProcessMessagingSkeleton: any,
        inProcessMessagingStub: InProcessMessagingStub,
        joynrMessage: any;

    beforeEach(done => {
        skeletonCallReturn = {
            key: "skeletonCallReturn"
        };
        inProcessMessagingSkeleton = {
            receiveMessage: jest.fn()
        };
        inProcessMessagingSkeleton.receiveMessage.mockReturnValue(skeletonCallReturn);
        inProcessMessagingStub = new InProcessMessagingStub(inProcessMessagingSkeleton);
        joynrMessage = {
            key: "joynrMessage"
        };
        done();
    });

    it("is instantiable and of correct type", () => {
        expect(InProcessMessagingStub).toBeDefined();
        expect(typeof InProcessMessagingStub === "function").toBeTruthy();
        expect(inProcessMessagingStub).toBeDefined();
        expect(inProcessMessagingStub instanceof InProcessMessagingStub).toBeTruthy();
        expect(inProcessMessagingStub.transmit).toBeDefined();
        expect(typeof inProcessMessagingStub.transmit === "function").toBeTruthy();
    });

    it("transmits a message", () => {
        const result = inProcessMessagingStub.transmit(joynrMessage);
        expect(inProcessMessagingSkeleton.receiveMessage).toHaveBeenCalledWith(joynrMessage);
        expect(result).toEqual(skeletonCallReturn);
    });
});
