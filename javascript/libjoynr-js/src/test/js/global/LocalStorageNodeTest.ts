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

import LocalStorage from "../../../main/js/global/LocalStorageNode";
import fs from "fs";
import path from "path";
// eslint-disable-next-line @typescript-eslint/camelcase
import child_process from "child_process";
import testUtil = require("../testUtil");

describe("local storage", () => {
    let storage: any;
    const item = {
        hi: "bla"
    };
    let testNum = 0;
    const testDirectory = "localStorageTestResults";
    const basePath = path.join(process.cwd(), testDirectory);
    let location: any;
    let locationPath: any;
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
            // eslint-disable-next-line @typescript-eslint/camelcase
            child_process.execSync(`rm -rf ${basePath}`);
        }
    });

    beforeEach(() => {
        testNum++;
        location = `${testDirectory}/LocalStorage-${testNum}`;
        locationPath = path.join(process.cwd(), location);
    });

    it("without clean directory", async () => {
        fs.mkdirSync(locationPath);
        const subDirectoryName = "SubdirectoryName";
        const subDirectoryLocation = path.join(locationPath, subDirectoryName);
        fs.mkdirSync(subDirectoryLocation);
        storage = new LocalStorage({
            clearPersistency: false,
            location
        });
        const e = await testUtil.reversePromise(storage.init());
        expect(e.message).toEqual(
            `joynr configuration error: Persistency subdirectory must not include other subdirectories. Directories found: ${JSON.stringify(
                [subDirectoryName]
            )}`
        );
    });

    describe("with clean directory", () => {
        beforeEach(async () => {
            storage = new LocalStorage({
                clearPersistency: false,
                location
            });
            await storage.init();
        });

        afterEach(async () => {
            await storage.shutdown();
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

        it("ignores corrupt files", async () => {
            storage.setItem(key, JSON.stringify(item));
            await storage.shutdown();
            const filename = fs.readdirSync(location)[0];
            fs.writeFileSync(`${location}/${filename}`, corruptData);

            storage = new LocalStorage({
                clearPersistency: false,
                location
            });
            await storage.init();
            expect(storage.getItem(key)).toBe(undefined);
        });

        it("overwrites corrupt files", async () => {
            storage.setItem(key, JSON.stringify(item));
            await storage.shutdown();
            const filename = fs.readdirSync(location)[0];
            fs.writeFileSync(`${location}/${filename}`, corruptData);

            storage = new LocalStorage({
                clearPersistency: false,
                location
            });

            await storage.init();
            storage.setItem(key, JSON.stringify(item));
            const files = fs.readdirSync(location);
            expect(files.length).toBe(1);
        });

        it("ignores other files", async () => {
            storage.setItem(key, JSON.stringify(item));
            await storage.shutdown();

            fs.writeFileSync(`${location}/otherFile`, "other Data");

            storage = new LocalStorage({
                clearPersistency: false,
                location
            });
            await storage.init();
            const files = fs.readdirSync(location);
            expect(files.length).toBe(2);
        });
    });
});
