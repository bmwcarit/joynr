/*jslint nomen: true */

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

define([ "joynr/messaging/MessagingStubFactory"
], function(MessagingStubFactory) {

    describe("libjoynr-js.joynr.messaging.MessagingStubFactory", function() {
        var messagingStub1, messagingStub2, factory1, factory2;
        var messagingStubFactory, address1, address2, address3;

        function Address1() {
            this._typeName = "Address1";
        }
        function Address2() {
            this._typeName = "Address2";
        }

        beforeEach(function(done) {
            messagingStub1 = {
                key : "messagingStub1"
            };
            messagingStub2 = {
                key : "messagingStub2"
            };

            factory1 = jasmine.createSpyObj("factory1", [ "build"
            ]);
            factory2 = jasmine.createSpyObj("factory1", [ "build"
            ]);

            factory1.build.and.returnValue(messagingStub1);
            factory2.build.and.returnValue(messagingStub2);

            messagingStubFactory = new MessagingStubFactory({
                messagingStubFactories : {
                    Address1 : factory1,
                    Address2 : factory2
                }
            });

            address1 = new Address1();
            address2 = new Address2();
            address3 = "";
            done();
        });

        it("is instantiable and has all members", function(done) {
            expect(MessagingStubFactory).toBeDefined();
            expect(typeof MessagingStubFactory === "function").toBeTruthy();
            expect(messagingStubFactory).toBeDefined();
            expect(messagingStubFactory instanceof MessagingStubFactory).toBeTruthy();
            expect(messagingStubFactory.createMessagingStub).toBeDefined();
            expect(typeof messagingStubFactory.createMessagingStub === "function").toBeTruthy();
            done();
        });

        it("it does not call any factory on creation", function(done) {
            expect(factory1.build).not.toHaveBeenCalled();
            expect(factory2.build).not.toHaveBeenCalled();
            done();
        });

        it("it resolves addresses correctly", function(done) {
            expect(messagingStubFactory.createMessagingStub(address1)).toEqual(messagingStub1);
            expect(messagingStubFactory.createMessagingStub(address2)).toEqual(messagingStub2);
            done();
        });

        it("it calls the build method of the factories", function(done) {
            messagingStubFactory.createMessagingStub(address1);
            expect(factory1.build).toHaveBeenCalledWith(address1);
            expect(factory2.build).not.toHaveBeenCalled();

            messagingStubFactory.createMessagingStub(address2);
            expect(factory2.build).toHaveBeenCalledWith(address2);
            done();
        });

        it("it throws on none-existing address", function(done) {
            expect(function() {
                messagingStubFactory.createMessagingStub(address3);
            }).toThrow();
            done();
        });

    });

});
/*jslint nomen: false */
