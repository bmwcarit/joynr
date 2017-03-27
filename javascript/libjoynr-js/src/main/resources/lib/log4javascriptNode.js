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
 * node wrapper for log4javascript
 *
 * @returns log4javascript wrapper for node.js
 */
define([], function() {
    var oldWindow = global.window;
    /* WORKAROUND: log4javascript node module v. 1.4.15 still expects a window object to be present */
    global.window = {};
    var result = require("log4javascript");
    global.window = oldWindow;
    return result;
});
