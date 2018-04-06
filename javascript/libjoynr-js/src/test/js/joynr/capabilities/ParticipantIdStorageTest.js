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
require("../../node-unit-test-helper");
var ParticipantIdStorage = require("../../../../main/js/joynr/capabilities/ParticipantIdStorage");
var MemoryStorage = require("../../../../main/js/global/MemoryStorage");

describe("libjoynr-js.joynr.capabilities.ParticipantIdStorage", function() {
    var participantIdStorage, localStorageSpy, uuidSpy;
    var domain, provider, interfaceName, key, uuid, storedParticipantId;
    var generatedParticipantId, localStorage;

    uuid = "uuid";
    interfaceName = "interface/Name";
    domain = "domain-1";
    provider = {
        interfaceName: interfaceName
    };
    storedParticipantId = "storedParticipantId";
    key = "joynr.participant." + domain + "." + interfaceName;
    generatedParticipantId = uuid;

    describe("with mocked LocalStorage", function() {
        var prepareTests = function(done) {
            var stored = null;
            var localStorageSpy = jasmine.createSpyObj("localStorageSpy", ["getItem", "setItem"]);
            localStorageSpy.getItem.and.callFake(function() {
                return stored;
            });
            localStorageSpy.setItem.and.callFake(function(key, value) {
                stored = value;
            });
            localStorage = localStorageSpy;
            uuidSpy = jasmine.createSpy("uuid");
            uuidSpy.and.returnValue(uuid);
            participantIdStorage = new ParticipantIdStorage(localStorage, uuidSpy);
            done();
        };
        sharedTests(prepareTests);
    });

    describe("with MemoryStorage", function() {
        var prepareTests = function(done) {
            localStorageSpy = null;
            uuidSpy = jasmine.createSpy("uuid");
            uuidSpy.and.returnValue(uuid);
            localStorage = new MemoryStorage();
            participantIdStorage = new ParticipantIdStorage(localStorage, uuidSpy);
            generatedParticipantId = uuid;
            done();
        };
        sharedTests(prepareTests);
    });

    function sharedTests(prepareTests) {
        beforeEach(prepareTests);

        it("is instantiable", function(done) {
            expect(participantIdStorage).toBeDefined();
            expect(participantIdStorage instanceof ParticipantIdStorage).toBeTruthy();
            done();
        });

        it("is has all members", function(done) {
            expect(participantIdStorage.getParticipantId).toBeDefined();
            expect(typeof participantIdStorage.getParticipantId === "function").toBeTruthy();
            done();
        });

        it("uses the stored participantId if available", function(done) {
            localStorage.setItem(key, storedParticipantId);
            var result = participantIdStorage.getParticipantId(domain, provider);
            if (localStorageSpy) {
                expect(localStorageSpy.getItem).toHaveBeenCalled();
                expect(localStorageSpy.getItem).toHaveBeenCalledWith(key);
                expect(localStorageSpy.setItem).not.toHaveBeenCalled();
            }
            expect(uuidSpy).not.toHaveBeenCalled();
            expect(result).toEqual(storedParticipantId);
            done();
        });

        it("generates a new uuid if no participantId is stored", function(done) {
            var result = participantIdStorage.getParticipantId(domain, provider);
            if (localStorageSpy) {
                expect(localStorageSpy.getItem).toHaveBeenCalled();
                expect(localStorageSpy.getItem).toHaveBeenCalledWith(key);
                expect(localStorageSpy.setItem).toHaveBeenCalled();
                expect(localStorageSpy.setItem).toHaveBeenCalledWith(key, generatedParticipantId);
            }
            expect(uuidSpy).toHaveBeenCalled();
            expect(uuidSpy).toHaveBeenCalledWith();
            expect(result).toEqual(generatedParticipantId);
            done();
        });
    }
});
