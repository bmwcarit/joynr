/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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
import MemoryStorage from "../../../main/js/global/MemoryStorage";

describe("memory storage", () => {
    let storage: any;
    const item = {
        hi: "bla"
    };
    const key = "key";

    beforeEach(done => {
        storage = new MemoryStorage();
        done();
    });

    it("can set and load item", () => {
        storage.setItem(key, JSON.stringify(item));
        const result = JSON.parse(storage.getItem(key));
        expect(result).toEqual(item);
    });

    it("can set and load long items", () => {
        const longString = new Array(200).join("a");

        storage.setItem(longString, JSON.stringify(item));
        const result = JSON.parse(storage.getItem(longString));
        expect(result).toEqual(item);
    });

    it("can remove items", () => {
        storage.setItem(key, JSON.stringify(item));
        storage.removeItem(key);
        const result = storage.getItem(key);
        expect(result).toEqual(undefined);
    });

    it("can clear items", () => {
        storage.setItem(key, JSON.stringify(item));
        storage.clear();
        const result = storage.getItem(key);
        expect(result).toEqual(undefined);
    });
});
