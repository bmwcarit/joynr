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
/**
 * @name MulticastWildcardRegexFactory
 * @constructor
 */
class MulticastWildcardRegexFactory {
    createIdPattern(multicastId) {
        let patternString = multicastId.replace(/^\+\//g, "[^/]+/");
        patternString = patternString.replace(/\/\+\//g, "/[^/]+/");
        patternString = patternString.replace(/([\w\W]*)\/[\\+]$/, "$1/[^/]+$");
        patternString = patternString.replace(/([\w\W]*)\/[\\*]$/, "$1(/.*)?$");
        if (patternString.length === 0 || patternString[patternString.length - 1] !== "$") {
            patternString += "$";
        }
        if (patternString.length === 0 || patternString[0] !== "^") {
            patternString = `^${patternString}`;
        }
        return patternString;
    }
}

module.exports = MulticastWildcardRegexFactory;
