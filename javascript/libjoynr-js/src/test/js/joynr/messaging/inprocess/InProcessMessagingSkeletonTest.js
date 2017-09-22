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
var InProcessMessagingSkeleton =
        require('../../../../classes/joynr/messaging/inprocess/InProcessMessagingSkeleton');
module.exports = (function(InProcessMessagingSkeleton) {

    describe("libjoynr-js.joynr.messaging.inprocess.InProcessMessagingSkeleton", function() {
        var listener, inProcessMessagingSkeleton, joynrMessage;

        beforeEach(function() {
            listener = jasmine.createSpy("listener");
            inProcessMessagingSkeleton = new InProcessMessagingSkeleton();
            inProcessMessagingSkeleton.registerListener(listener);
            joynrMessage = {
                key : "joynrMessage"
            };
        });

        it("is instantiable and of correct type", function() {
            expect(InProcessMessagingSkeleton).toBeDefined();
            expect(typeof InProcessMessagingSkeleton === "function").toBeTruthy();
            expect(inProcessMessagingSkeleton).toBeDefined();
            expect(inProcessMessagingSkeleton instanceof InProcessMessagingSkeleton).toBeTruthy();
            expect(inProcessMessagingSkeleton.receiveMessage).toBeDefined();
            expect(typeof inProcessMessagingSkeleton.receiveMessage === "function").toBeTruthy();
            expect(inProcessMessagingSkeleton.registerListener).toBeDefined();
            expect(typeof inProcessMessagingSkeleton.registerListener === "function").toBeTruthy();
        });

        it("transmits a message", function() {
            inProcessMessagingSkeleton.receiveMessage(joynrMessage);
            expect(listener).toHaveBeenCalledWith(joynrMessage);
        });

    });

}(InProcessMessagingSkeleton));