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

const TypingMock = { isEnumType: jest.fn() };
jest.doMock("../../../../main/js/joynr/util/Typing", () => {
    return TypingMock;
});

import * as JSONSerializer from "../../../../main/js/joynr/util/JSONSerializer";

describe("libjoynr-js.joynr.JSONSerializer.ensureCorrectSerialization", () => {
    let fixture: Record<string, any>;
    beforeEach(() => {
        jest.clearAllMocks();
        fixture = { name: "mock" };
        TypingMock.isEnumType.mockReturnValue(true);
    });

    it("Test enum serialization", () => {
        const actual = JSONSerializer.stringify(fixture);

        expect(TypingMock.isEnumType).toHaveBeenCalled();
        expect(actual).toBe(`"${fixture.name}"`);
    });

    it("Test no enum serialization", () => {
        TypingMock.isEnumType.mockReturnValue(false);
        const actual = JSONSerializer.stringify(fixture);

        expect(TypingMock.isEnumType).toHaveBeenCalled();
        expect(actual).toEqual(JSON.stringify(fixture));
    });

    it("stringifyOptional false Test enum serialization", () => {
        const actual = JSONSerializer.stringifyOptional(fixture, false);

        expect(TypingMock.isEnumType).toHaveBeenCalled();
        expect(actual).toBe(`"${fixture.name}"`);
    });

    it("stringifyOptional false Test no enum serialization", () => {
        TypingMock.isEnumType.mockReturnValue(false);
        const actual = JSONSerializer.stringifyOptional(fixture, false);

        expect(TypingMock.isEnumType).toHaveBeenCalled();
        expect(actual).toEqual(JSON.stringify(fixture));
    });

    it("stringifyOptional true", () => {
        const actual = JSONSerializer.stringifyOptional(fixture, true);
        expect(TypingMock.isEnumType).not.toHaveBeenCalled();
        expect(actual).toEqual(JSON.stringify(fixture));
    });
});
