/*global joynrTestRequire: true */

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

joynrTestRequire(
        "joynr/messaging/inprocess/TestInProcessMessagingStub",
        [ "joynr/messaging/inprocess/InProcessMessagingStub"
        ],
        function(InProcessMessagingStub) {

            describe(
                    "libjoynr-js.joynr.messaging.inprocess.InProcessMessagingStub",
                    function() {
                        var skeletonCallReturn, inProcessMessagingSkeleton, inProcessMessagingStub, joynrMessage;

                        beforeEach(function() {
                            skeletonCallReturn = {
                                key : "skeletonCallReturn"
                            };
                            inProcessMessagingSkeleton =
                                    jasmine.createSpyObj(
                                            "inProcessMessagingSkeleton",
                                            [ "receiveMessage"
                                            ]);
                            inProcessMessagingSkeleton.receiveMessage.andReturn(skeletonCallReturn);
                            inProcessMessagingStub =
                                    new InProcessMessagingStub(inProcessMessagingSkeleton);
                            joynrMessage = {
                                key : "joynrMessage"
                            };
                        });

                        it("is instantiable and of correct type", function() {
                            expect(InProcessMessagingStub).toBeDefined();
                            expect(typeof InProcessMessagingStub === "function").toBeTruthy();
                            expect(inProcessMessagingStub).toBeDefined();
                            expect(inProcessMessagingStub instanceof InProcessMessagingStub)
                                    .toBeTruthy();
                            expect(inProcessMessagingStub.transmit).toBeDefined();
                            expect(typeof inProcessMessagingStub.transmit === "function")
                                    .toBeTruthy();
                        });

                        it("transmits a message", function() {
                            var result = inProcessMessagingStub.transmit(joynrMessage);
                            expect(inProcessMessagingSkeleton.receiveMessage).toHaveBeenCalledWith(
                                    joynrMessage);
                            expect(result).toEqual(skeletonCallReturn);
                        });

                    });

        });