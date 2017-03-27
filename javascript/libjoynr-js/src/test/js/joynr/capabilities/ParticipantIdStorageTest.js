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

//TODO: some of this relies on the dummy implementation, change accordingly when implementating
define([ "joynr/capabilities/ParticipantIdStorage"
], function(ParticipantIdStorage) {
    describe("libjoynr-js.joynr.capabilities.ParticipantIdStorage", function() {
        var participantIdStorage, localStorageSpy, uuidSpy;
        var domain, provider, interfaceName, key, uuid, storedParticipantId;
        var generatedParticipantId;

        beforeEach(function(done) {
            uuid = "uuid";
            interfaceName = "interface/Name";
            domain = "domain-1";
            provider = {
                interfaceName : interfaceName
            };
            var interfaceNameDotted = interfaceName.replace("/", ".");
            localStorageSpy = jasmine.createSpyObj("localStorageSpy", [
                "getItem",
                "setItem"
            ]);
            uuidSpy = jasmine.createSpy("uuid");
            uuidSpy.and.returnValue(uuid);
            participantIdStorage = new ParticipantIdStorage(localStorageSpy, uuidSpy);
            key = "joynr.participant." + domain + "." + interfaceNameDotted;
            generatedParticipantId = domain + "." + interfaceNameDotted + "." + uuid;
            storedParticipantId = "storedParticipantId";
            done();
        });

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
            localStorageSpy.getItem.and.returnValue(storedParticipantId);
            var result = participantIdStorage.getParticipantId(domain, provider);
            expect(localStorageSpy.getItem).toHaveBeenCalled();
            expect(localStorageSpy.getItem).toHaveBeenCalledWith(key);
            expect(uuidSpy).not.toHaveBeenCalled();
            expect(localStorageSpy.setItem).not.toHaveBeenCalled();
            expect(result).toEqual(storedParticipantId);
            done();
        });

        it("generates a new uuid if no participantId is stored", function(done) {
            var result = participantIdStorage.getParticipantId(domain, provider);
            expect(localStorageSpy.getItem).toHaveBeenCalled();
            expect(localStorageSpy.getItem).toHaveBeenCalledWith(key);
            expect(uuidSpy).toHaveBeenCalled();
            expect(uuidSpy).toHaveBeenCalledWith();
            expect(localStorageSpy.setItem).toHaveBeenCalled();
            expect(localStorageSpy.setItem).toHaveBeenCalledWith(key, generatedParticipantId);
            expect(result).toEqual(generatedParticipantId);
            done();
        });
    });

}); // require
