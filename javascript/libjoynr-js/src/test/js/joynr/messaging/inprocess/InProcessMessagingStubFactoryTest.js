/*jslint node: true */

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
var InProcessMessagingStubFactory = require("../../../../../main/js/joynr/messaging/inprocess/InProcessMessagingStubFactory");

describe("libjoynr-js.joynr.messaging.inprocess.InProcessMessagingStubFactory", function() {
    var skeletonCallReturn, inProcessMessagingSkeleton, inProcessAddress;
    var inProcessMessagingStubFactory, joynrMessage;

    beforeEach(function(done) {
        skeletonCallReturn = {
            key: "skeletonCallReturn"
        };
        inProcessMessagingSkeleton = jasmine.createSpyObj("inProcessMessagingSkeleton", ["receiveMessage"]);
        inProcessMessagingSkeleton.receiveMessage.and.returnValue(skeletonCallReturn);
        inProcessAddress = jasmine.createSpyObj("inProcessAddress", ["getSkeleton"]);
        inProcessAddress.getSkeleton.and.returnValue(inProcessMessagingSkeleton);
        inProcessMessagingStubFactory = new InProcessMessagingStubFactory({});
        joynrMessage = {
            key: "joynrMessage"
        };
        done();
    });

    it("is instantiable and of correct type", function(done) {
        expect(InProcessMessagingStubFactory).toBeDefined();
        expect(typeof InProcessMessagingStubFactory === "function").toBeTruthy();
        expect(inProcessMessagingStubFactory).toBeDefined();
        expect(inProcessMessagingStubFactory instanceof InProcessMessagingStubFactory).toBeTruthy();
        expect(inProcessMessagingStubFactory.build).toBeDefined();
        expect(typeof inProcessMessagingStubFactory.build === "function").toBeTruthy();
        done();
    });

    it("creates a messaging stub and uses it correctly", function(done) {
        var inProcessMessagingStub = inProcessMessagingStubFactory.build(inProcessAddress);
        expect(inProcessAddress.getSkeleton).toHaveBeenCalledWith();

        var result = inProcessMessagingStub.transmit(joynrMessage);
        expect(inProcessMessagingSkeleton.receiveMessage).toHaveBeenCalledWith(joynrMessage);
        expect(result).toEqual(skeletonCallReturn);
        done();
    });
});
