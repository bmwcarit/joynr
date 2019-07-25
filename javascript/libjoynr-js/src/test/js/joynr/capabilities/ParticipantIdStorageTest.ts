/*eslint no-use-before-define: "off"*/
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

import ParticipantIdStorage from "../../../../main/js/joynr/capabilities/ParticipantIdStorage";
import MemoryStorage from "../../../../main/js/global/MemoryStorage";
import * as CapabilitiesUtil from "../../../../main/js/joynr/util/CapabilitiesUtil";

describe("libjoynr-js.joynr.capabilities.ParticipantIdStorage", () => {
    let participantIdStorage: ParticipantIdStorage, localStorageSpy: any, nanoidSpy: any;
    let generatedParticipantId: any, localStorage: any;

    const nanoid = "nanoid";
    const interfaceName = "interface/Name";
    const domain = "domain-1";
    const provider = {
        interfaceName,
        constructor: {
            MAJOR_VERSION: 1
        }
    };
    const storedParticipantId = "storedParticipantId";
    const key = CapabilitiesUtil.generateParticipantIdStorageKey(
        domain,
        interfaceName,
        provider.constructor.MAJOR_VERSION
    );
    generatedParticipantId = nanoid;

    describe("with mocked LocalStorage", () => {
        const prepareTests = function() {
            let stored: any = null;
            const localStorageSpy = {
                getItem: jest.fn(),
                setItem: jest.fn()
            };
            localStorageSpy.getItem.mockImplementation(() => {
                return stored;
            });
            localStorageSpy.setItem.mockImplementation((_key: string, value: any) => {
                stored = value;
            });
            localStorage = localStorageSpy;
            nanoidSpy = jest.fn();
            nanoidSpy.mockReturnValue(nanoid);
            participantIdStorage = new ParticipantIdStorage(localStorage, nanoidSpy);
        };
        sharedTests(prepareTests);
    });

    describe("with MemoryStorage", () => {
        const prepareTests = function() {
            localStorageSpy = null;
            nanoidSpy = jest.fn();
            nanoidSpy.mockReturnValue(nanoid);
            localStorage = new MemoryStorage();
            participantIdStorage = new ParticipantIdStorage(localStorage, nanoidSpy);
            generatedParticipantId = nanoid;
        };
        sharedTests(prepareTests);
    });

    function sharedTests(prepareTests: Function) {
        beforeEach(() => prepareTests());

        it("is instantiable", () => {
            expect(participantIdStorage).toBeDefined();
        });

        it("is has all members", () => {
            expect(participantIdStorage.getParticipantId).toBeDefined();
            expect(typeof participantIdStorage.getParticipantId === "function").toBeTruthy();
        });

        it("uses the stored participantId if available", () => {
            localStorage.setItem(key, storedParticipantId);
            const result = participantIdStorage.getParticipantId(domain, provider as any);
            if (localStorageSpy) {
                expect(localStorageSpy.getItem).toHaveBeenCalled();
                expect(localStorageSpy.getItem).toHaveBeenCalledWith(key);
                expect(localStorageSpy.setItem).not.toHaveBeenCalled();
            }
            expect(nanoidSpy).not.toHaveBeenCalled();
            expect(result).toEqual(storedParticipantId);
        });

        it("stores participantId when setParticipantId is called", () => {
            participantIdStorage.setParticipantId(domain, provider as any, storedParticipantId);
            if (localStorageSpy) {
                expect(localStorageSpy.setItem).toHaveBeenCalledWith(key, storedParticipantId);
            }

            const result = participantIdStorage.getParticipantId(domain, provider as any);
            if (localStorageSpy) {
                expect(localStorageSpy.getItem).toHaveBeenCalled();
                expect(localStorageSpy.getItem).toHaveBeenCalledWith(key);
            }
            expect(nanoidSpy).not.toHaveBeenCalled();
            expect(result).toEqual(storedParticipantId);
        });

        it("generates a new unique identifier if no participantId is stored", () => {
            const result = participantIdStorage.getParticipantId(domain, provider as any);
            if (localStorageSpy) {
                expect(localStorageSpy.getItem).toHaveBeenCalled();
                expect(localStorageSpy.getItem).toHaveBeenCalledWith(key);
                expect(localStorageSpy.setItem).toHaveBeenCalled();
                expect(localStorageSpy.setItem).toHaveBeenCalledWith(key, generatedParticipantId);
            }
            expect(nanoidSpy).toHaveBeenCalled();
            expect(nanoidSpy).toHaveBeenCalledWith();
            expect(result).toEqual(generatedParticipantId);
        });
    }
});
