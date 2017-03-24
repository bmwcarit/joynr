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

/**
 * Requirejs helper function for jasmine-node.
 *
 * The test framework jasmine-node uses different vm contexts during test definition and test execution. These different contexts do not
 * share the same global general-purpose constructor Date. To always use the same Date general-purpose constructor, require on this file and
 * overwrite your local Date general-purpose constructor with the returned one.
 *
 * @returns the global Date general-purpose constructor.
 */
define(function() {
    return Date;
});
