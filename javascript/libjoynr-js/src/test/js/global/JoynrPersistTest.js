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
require("../node-unit-test-helper");
const JoynrPersist = require("../../../main/js/global/JoynrPersist");
const fs = require("fs");
const path = require("path");
const child_process = require("child_process");

describe("global.JoynrPersist", () => {
    let persist;
    const item = {
        hi: "bla"
    };
    let testNum = 0;
    const testDirectory = "joynrPersistTestResults";
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
            child_process.execSync("rm -rf " + basePath);
        }
    });

    beforeEach(() => {
        testNum++;
        location = testDirectory + "/LocalStorage-" + testNum;
        locationPath = path.join(process.cwd(), location);
    });

    async function createJoynrPersist() {
        persist = new JoynrPersist({
            dir: location
        });
        return await persist.init();
    }

    it("without clean directory", async () => {
        fs.mkdirSync(locationPath);
        const subDirectoryName = "SubdirectoryName";
        const subDirectoryLocation = path.join(locationPath, subDirectoryName);
        fs.mkdirSync(subDirectoryLocation);

        try {
            await createJoynrPersist();
        } catch (e) {
            expect(e.message).toEqual(
                "joynr configuration error: Persistency subdirectory must not include other subdirectories. Directories found: " +
                    JSON.stringify([subDirectoryName])
            );
            return;
        }
        return Promise.reject(new Error("JoynrPersist.init did not throw"));
    });

    describe("with clean directory", () => {
        beforeEach(createJoynrPersist);

        it("can set and load item", async () => {
            await persist.setItem(key, item);
            const result = await persist.getAllData(key);
            expect(result[0].value).toEqual(item);
        });

        it("can set and load long keys", async () => {
            const longString = new Array(200).join("a");

            await persist.setItem(longString, item);
            const result = await persist.getAllData(longString);
            expect(result[0].value).toEqual(item);
        });

        it("can remove items", async () => {
            await persist.setItem(key, item);
            await persist.removeItem(key);
            const result = await persist.getAllData(key);
            expect(result.length).toEqual(0);
        });

        it("can clear items", async () => {
            await persist.setItem(key, item);
            await persist.clear();
            const result = await persist.getAllData(key);
            expect(result.length).toEqual(0);
        });

        it("ignores corrupt files", async () => {
            await persist.setItem(key, item);

            const filename = fs.readdirSync(location)[0];
            fs.writeFileSync(location + "/" + filename, corruptData);

            await createJoynrPersist();
            const result = await persist.getAllData(key);
            expect(result.length).toBe(0);
        });

        it("overwrites corrupt files", async () => {
            await persist.setItem(key, item);

            const filename = fs.readdirSync(location)[0];
            fs.writeFileSync(location + "/" + filename, corruptData);

            await createJoynrPersist();

            await persist.setItem(key, item);
            const files = fs.readdirSync(location);
            expect(files.length).toBe(1);
        });

        it("ignores other files", async () => {
            await persist.setItem(key, item);

            fs.writeFileSync(location + "/otherFile", "other Data");

            await createJoynrPersist();
            const files = fs.readdirSync(location);
            expect(files.length).toBe(2);
            const data = await persist.getAllData();
            expect(data.length).toBe(1);
        });
    });
});
