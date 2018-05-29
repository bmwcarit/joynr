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

const crypto = require("crypto");
const fs = require("fs");
const path = require("path");
const { promisify } = require("util");
const encoding = "utf8";
const LoggingManager = require("../joynr/system/LoggingManager");
const log = LoggingManager.getLogger("joynr.global.JoynrPersist");

const readFileAsync = promisify(fs.readFile);
const writeFileAsync = promisify(fs.writeFile);
const readDirAsync = promisify(fs.readdir);
const lstatAsync = promisify(fs.lstat);
const mkdirAsync = promisify(fs.mkdir);
const unlinkAsync = promisify(fs.unlink);

function hashFileName(key) {
    return crypto
        .createHash("md5")
        .update(key)
        .digest("hex");
}

const resolveDir = function(dir) {
    dir = path.normalize(dir);
    if (path.isAbsolute(dir)) {
        return dir;
    }
    return path.join(process.cwd(), dir);
};

async function mkdirRecursive(directory) {
    try {
        await mkdirAsync(directory);
    } catch (e) {
        if (e.code === "ENOENT") {
            const parentDir = path.dirname(directory);
            await mkdirRecursive(parentDir);
            await mkdirAsync(directory);
        } else {
            throw e;
        }
    }
}

function JoynrPersist(options) {
    this._directory = resolveDir(options.dir);
}

JoynrPersist.prototype = {
    async init() {
        await lstatAsync(this._directory).catch(() => mkdirRecursive(this._directory));
        return this.getAllData().catch(e => {
            if (e.code === "EISDIR") {
                const files = fs.readdirSync(this._directory);
                const subDirectories = files.filter(file => {
                    const filePath = path.join(this._directory, file);
                    return fs.lstatSync(filePath).isDirectory();
                });
                throw new Error(
                    `joynr configuration error: Persistency subdirectory must not include other subdirectories. Directories found: ${JSON.stringify(
                        subDirectories
                    )}`
                );
            } else {
                throw e;
            }
        });
    },

    getPath(key) {
        return path.join(this._directory, hashFileName(key));
    },

    getAllData() {
        return readDirAsync(this._directory)
            .then(files => {
                const promises = [];
                for (let i = 0, length = files.length; i < length; i++) {
                    const currentFile = files[i];
                    const filePath = path.join(this._directory, currentFile);
                    /*eslint-disable promise/no-nesting*/
                    promises.push(
                        readFileAsync(filePath, encoding)
                            .then(JSON.parse)
                            .catch(e => {
                                if (e.code === "EISDIR") {
                                    throw e;
                                }
                                log.warn(
                                    `corrupted files in JoynrPersist directory: unable to parse ${filePath}. Ignoring file. Error: ${e}`
                                );
                                return null;
                            })
                    );
                    /*eslint-enable promise/no-nesting*/
                }
                return Promise.all(promises);
            })
            .then(arr => {
                return arr.filter(element => element !== null);
            });
    },

    setItem(key, value) {
        const datum = { key, value };
        return writeFileAsync(this.getPath(key), JSON.stringify(datum));
    },

    removeItem(key) {
        return unlinkAsync(this.getPath(key));
    },

    async clear() {
        // unfortunately we can't spawn a child process and call rm -rf
        // TOTO: check if it's bad to delete all files here.
        return readDirAsync(this._directory).then(files => {
            const promises = [];
            for (let i = 0, length = files.length; i < length; i++) {
                const currentFile = files[i];
                promises.push(unlinkAsync(path.join(this._directory, currentFile)));
            }
            return Promise.all(promises);
        });
    }
};

module.exports = JoynrPersist;
