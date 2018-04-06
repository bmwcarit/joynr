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
require("../../../node-unit-test-helper");
var InProcessMessagingStub = require("../../../../../main/js/joynr/messaging/inprocess/InProcessMessagingStub");

describe("libjoynr-js.joynr.messaging.inprocess.InProcessMessagingStub", function() {
    var skeletonCallReturn, inProcessMessagingSkeleton, inProcessMessagingStub, joynrMessage;

    beforeEach(function(done) {
        skeletonCallReturn = {
            key: "skeletonCallReturn"
        };
        inProcessMessagingSkeleton = jasmine.createSpyObj("inProcessMessagingSkeleton", ["receiveMessage"]);
        inProcessMessagingSkeleton.receiveMessage.and.returnValue(skeletonCallReturn);
        inProcessMessagingStub = new InProcessMessagingStub(inProcessMessagingSkeleton);
        joynrMessage = {
            key: "joynrMessage"
        };
        done();
    });

    it("is instantiable and of correct type", function(done) {
        expect(InProcessMessagingStub).toBeDefined();
        expect(typeof InProcessMessagingStub === "function").toBeTruthy();
        expect(inProcessMessagingStub).toBeDefined();
        expect(inProcessMessagingStub instanceof InProcessMessagingStub).toBeTruthy();
        expect(inProcessMessagingStub.transmit).toBeDefined();
        expect(typeof inProcessMessagingStub.transmit === "function").toBeTruthy();
        done();
    });

    it("transmits a message", function(done) {
        var result = inProcessMessagingStub.transmit(joynrMessage);
        expect(inProcessMessagingSkeleton.receiveMessage).toHaveBeenCalledWith(joynrMessage);
        expect(result).toEqual(skeletonCallReturn);
        done();
    });
});
