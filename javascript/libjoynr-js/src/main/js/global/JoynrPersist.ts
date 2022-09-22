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
import crypto from "crypto";

import fs from "fs";
import path from "path";
import { promisify } from "util";
const encoding = "utf8";
import LoggingManager from "../joynr/system/LoggingManager";
const log = LoggingManager.getLogger("joynr.global.JoynrPersist");

const readFileAsync = promisify(fs.readFile);
const writeFileAsync = promisify(fs.writeFile);
const readDirAsync = promisify(fs.readdir);
const lstatAsync = promisify(fs.lstat);
const mkdirAsync = promisify(fs.mkdir);
const unlinkAsync = promisify(fs.unlink);

function hashFileName(key: string): string {
    return crypto
        .createHash("md5")
        .update(key)
        .digest("hex");
}

const resolveDir = function(dir: string): string {
    dir = path.normalize(dir);
    if (path.isAbsolute(dir)) {
        return dir;
    }
    return path.join(process.cwd(), dir);
};

async function mkdirRecursive(directory: string): Promise<void> {
    try {
        await mkdirAsync(directory);
    } catch (e) {
        if (e.code === "ENOENT") {
            const parentDir = path.dirname(directory);
            await mkdirRecursive(parentDir);
            try {
                await mkdirAsync(directory);
            } catch (err) {
                if (err.code === "EEXIST") {
                    log.info(`Directory: ${directory} already exists`);
                } else {
                    throw err;
                }
            }
        } else if (e.code === "EEXIST") {
            log.info(`Directory: ${directory} already exists`);
        } else {
            throw e;
        }
    }
}

class JoynrPersist {
    private directory: string;
    public constructor(options: { dir: string }) {
        this.directory = resolveDir(options.dir);
    }

    public async init(): Promise<{ key: string; value: any }[]> {
        await lstatAsync(this.directory).catch(() => mkdirRecursive(this.directory));
        return this.getAllData().catch(e => {
            if (e.code === "EISDIR") {
                const files = fs.readdirSync(this.directory);
                const subDirectories = files.filter(file => {
                    const filePath = path.join(this.directory, file);
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
    }

    private getPath(key: string): string {
        return path.join(this.directory, hashFileName(key));
    }

    public getAllData(): Promise<any> {
        return readDirAsync(this.directory)
            .then(files => {
                const promises = [];
                for (let i = 0, length = files.length; i < length; i++) {
                    const currentFile = files[i];
                    const filePath = path.join(this.directory, currentFile);
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
    }

    public setItem(key: string, value: any): Promise<void> {
        const datum = { key, value };
        return writeFileAsync(this.getPath(key), JSON.stringify(datum));
    }

    public removeItem(key: string): Promise<void> {
        return unlinkAsync(this.getPath(key));
    }

    public async clear(): Promise<any> {
        // unfortunately we can't spawn a child process and call rm -rf
        // TOTO: check if it's bad to delete all files here.
        return readDirAsync(this.directory).then(files => {
            const promises = [];
            for (let i = 0, length = files.length; i < length; i++) {
                const currentFile = files[i];
                promises.push(unlinkAsync(path.join(this.directory, currentFile)));
            }
            return Promise.all(promises);
        });
    }
}

export = JoynrPersist;
