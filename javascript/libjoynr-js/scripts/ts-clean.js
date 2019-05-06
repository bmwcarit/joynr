#!/usr/bin/env node

/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
"use strict";

const fs = require("fs");
const util = require("util");
const path = require("path");
const glob = require("glob");
const tsConfig = require("../tsconfig.json");

const globAsync = util.promisify(glob);
const unlinkAsync = util.promisify(fs.unlink);

const validOption = ["all", "test", "main"];
const errorMessage = `Error: Missing argument.\nUsage: ${path.basename(__filename)} ${validOption.join(" | ")}`;

const removeFile = file =>
    unlinkAsync(file)
        .then(() => file)
        .catch(err => (err.code === "ENOENT" ? null : Promise.reject(err)));

/**
 * Remove Typescript output files by replacing extension .ts -> .js
 *
 * @returns {Array<string>} Deleted files.
 * @param {string} option One of: all, src, tests.
 */
function tsClean(option) {
    if (!validOption.includes(option)) {
        return Promise.reject(new Error(errorMessage));
    }

    if (!Array.isArray(tsConfig.include)) {
        return Promise.reject(new Error("tsConfig.include is not an Array"));
    }

    const folders = tsConfig.include.filter(globPattern => option === "all" || globPattern.includes(option));

    return Promise.all(
        folders.map(includeMatch => {
            return globAsync(`${includeMatch}.ts`, { ignore: `${includeMatch}.d.ts` }).then(files => {
                return Promise.all(
                    files.map(file => {
                        const generatedFiles = [
                            file.replace(/.ts$/, ".js"),
                            file.replace(/.ts$/, ".js.map"),
                            file.replace(/.ts$/, ".d.ts")
                        ];

                        return Promise.all(generatedFiles.map(removeFile));
                    })
                );
            });
        })
    ).then(filesMatched =>
        Array.prototype.concat(
            ...filesMatched.map(generatedFiles => {
                return Array.prototype.concat(...generatedFiles.map(files => files.filter(Boolean)));
            })
        )
    );
}

if (module.parent) {
    module.exports = { tsClean };
} else {
    /*eslint-disable no-console*/
    if (process.argv.length !== 3) {
        console.error(errorMessage);
        process.exit(1);
    }
    tsClean(process.argv[2])
        .then(filesRemoved => filesRemoved.forEach(file => console.log(file)))
        .catch(err => {
            console.error(err);
            process.exit(1);
        });
}
