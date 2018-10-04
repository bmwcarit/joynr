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

const RequireUtil = {};

/**
 * This method deletes the given module and all modules that where
 * required by it from the require cache. Modules that where required before
 * are not touched.
 * So the execution of this method allows for a reset of the cache to the
 * state from before the module was required.
 *
 * @function
 * @name RequireUtil#deleteModuleAndNewChildrenFromCache
 *
 * @param {String}
 *            resolvedPath absolute path to the storage file of the required
 *                         module, which also serves as key for the module in
 *                         the require cache
 */
RequireUtil.deleteModuleAndNewChildrenFromCache = resolvedPath => {
    let current;
    if ((current = require.cache[resolvedPath])) {
        const children = require.cache[resolvedPath].children;
        children.forEach(child => {
            let childInCache;
            if ((childInCache = require.cache[child.id]) && childInCache.parent.id === current.id) {
                RequireUtil.deleteModuleAndNewChildrenFromCache(childInCache.id);
            }
        });
        delete require.cache[resolvedPath];
    }
};

/**
 * This method returns a string that, when evaluated, requires the module at
 * the given absolute path and saves the result in the variable with the given
 * name for each entry of the given map.
 *
 * @function
 * @name RequireUtil#safeRequire
 *
 * @param {Map}
 *            map map that stores name-path pairs with the name of the
 *                variable where the loaded module is to be stored in as key
 *                and the absolute path of the module's storage file as value.
 *
 * @return {String} a string that requires the modules when it is evaluated
 */

RequireUtil.safeRequire = map => {
    let evalStr = ``;
    for (const [name, path] of map.entries()) {
        evalStr = `${evalStr}${name} = require("${path}");`;
    }
    return evalStr;
};

/**
 * This method iterates over all the elements of the given map and for each
 * element deletes the module at the given path and its children from the
 * cache (using RequireUtil.deleteModuleAndNewChildrenFromCache).
 *
 * @function
 * @name RequireUtil#deleteFromCache
 *
 * @param {Map} map map with name-path pairs as elements
 */
RequireUtil.deleteFromCache = map => {
    for (const path of map.values()) {
        RequireUtil.deleteModuleAndNewChildrenFromCache(path);
    }
};

module.exports = RequireUtil;
