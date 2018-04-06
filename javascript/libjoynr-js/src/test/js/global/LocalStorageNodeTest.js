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
var LocalStorage = require("../../../main/js/global/LocalStorageNode");
var fs = require("fs");

describe("local storage", function() {
    var storage;
    var item = {
        hi: "bla"
    };
    var testNum = 0;
    var location;
    var key = "key";
    var corruptData = "corrupted Data";

    afterEach(function() {
        fs.readdirSync(location).forEach(function(file) {
            var filePath = location + "/" + file;
            fs.unlinkSync(filePath);
        });
        fs.rmdirSync(location);
    });

    beforeEach(function(done) {
        testNum++;
        location = "localStorageTestResults/LocalStorage-" + testNum;
        storage = new LocalStorage({
            clearPersistency: false,
            location: location
        });
        done();
    });

    it("can set and load item", function() {
        storage.setItem(key, JSON.stringify(item));
        var result = JSON.parse(storage.getItem(key));
        expect(result).toEqual(item);
    });

    it("can set and load long items", function() {
        var longString = new Array(200).join("a");

        storage.setItem(longString, JSON.stringify(item));
        var result = JSON.parse(storage.getItem(longString));
        expect(result).toEqual(item);
    });

    it("can remove items", function() {
        storage.setItem(key, JSON.stringify(item));
        storage.removeItem(key);
        var result = storage.getItem(key);
        expect(result).toEqual(null);
    });

    it("can clear items", function() {
        storage.setItem(key, JSON.stringify(item));
        storage.clear();
        var result = storage.getItem(key);
        expect(result).toEqual(null);
    });

    it("ignores corrupt files", function() {
        storage.setItem(key, JSON.stringify(item));
        var filename = fs.readdirSync(location)[0];
        fs.writeFileSync(location + "/" + filename, corruptData);

        storage = new LocalStorage({
            clearPersistency: false,
            location: location
        });
        expect(storage.getItem(key)).toBe(null);
    });

    it("overwrites corrupt files", function() {
        storage.setItem(key, JSON.stringify(item));
        var filename = fs.readdirSync(location)[0];
        fs.writeFileSync(location + "/" + filename, corruptData);

        storage = new LocalStorage({
            clearPersistency: false,
            location: location
        });

        storage.setItem(key, JSON.stringify(item));
        var files = fs.readdirSync(location);
        expect(files.length).toBe(1);
    });

    it("ignores other files", function() {
        storage.setItem(key, JSON.stringify(item));
        fs.writeFileSync(location + "/otherFile", "other Data");

        storage = new LocalStorage({
            clearPersistency: false,
            location: location
        });

        var files = fs.readdirSync(location);
        expect(files.length).toBe(2);
    });
});
