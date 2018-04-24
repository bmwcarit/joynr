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
require("../node-unit-test-helper");
const LocalStorage = require("../../../main/js/global/LocalStorageNode");
const fs = require("fs");
const path = require("path");

describe("local storage", () => {
    let storage;
    const item = {
        hi: "bla"
    };
    let testNum = 0;
    const testDirectory = "localStorageTestResults";
    const basePath = path.join(process.cwd(), testDirectory);
    let location;
    let locationPath;
    const key = "key";
    const corruptData = "corrupted Data";
    const clearResults = true;

    beforeAll(() => {
        if (!fs.existsSync(basePath)) {
            fs.mkdirSync(basePath);
        }
    });

    afterAll(() => {
        if (clearResults) {
            fs.rmdirSync(basePath);
        }
    });

    beforeEach(() => {
        testNum++;
        location = testDirectory + "/LocalStorage-" + testNum;
        locationPath = path.join(process.cwd(), location);
    });

    afterEach(() => {
        if (clearResults) {
            fs.readdirSync(locationPath).forEach(file => {
                const filePath = locationPath + "/" + file;
                fs.unlinkSync(filePath);
            });
            fs.rmdirSync(locationPath);
        }
    });

    it("without clean directory", () => {
        fs.mkdirSync(locationPath);
        const subDirectoryName = "SubdirectoryName";
        const subDirectoryLocation = path.join(locationPath, subDirectoryName);
        fs.mkdirSync(subDirectoryLocation);
        expect(() => {
            storage = new LocalStorage({
                clearPersistency: false,
                location
            });
        }).toThrow(
            new Error(
                "joynr configuration error: Persistency subdirectory must not include other subdirectories. Directories found: " +
                    JSON.stringify([subDirectoryName])
            )
        );
        fs.rmdirSync(subDirectoryLocation);
    });

    describe("with clean directory", () => {
        beforeEach(() => {
            storage = new LocalStorage({
                clearPersistency: false,
                location
            });
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
            expect(result).toEqual(null);
        });

        it("can clear items", () => {
            storage.setItem(key, JSON.stringify(item));
            storage.clear();
            const result = storage.getItem(key);
            expect(result).toEqual(null);
        });

        it("ignores corrupt files", () => {
            storage.setItem(key, JSON.stringify(item));
            const filename = fs.readdirSync(location)[0];
            fs.writeFileSync(location + "/" + filename, corruptData);

            storage = new LocalStorage({
                clearPersistency: false,
                location
            });
            expect(storage.getItem(key)).toBe(null);
        });

        it("overwrites corrupt files", () => {
            storage.setItem(key, JSON.stringify(item));
            const filename = fs.readdirSync(location)[0];
            fs.writeFileSync(location + "/" + filename, corruptData);

            storage = new LocalStorage({
                clearPersistency: false,
                location
            });

            storage.setItem(key, JSON.stringify(item));
            const files = fs.readdirSync(location);
            expect(files.length).toBe(1);
        });

        it("ignores other files", () => {
            storage.setItem(key, JSON.stringify(item));
            fs.writeFileSync(location + "/otherFile", "other Data");

            storage = new LocalStorage({
                clearPersistency: false,
                location
            });

            const files = fs.readdirSync(location);
            expect(files.length).toBe(2);
        });
    });
});
