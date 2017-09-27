/*jslint es5: true, nomen: true, node: true */
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

var Typing = require('../../util/Typing');
var BrowserMessagingStub = require('./BrowserMessagingStub');
module.exports =
        (function(Typing, BrowserMessagingStub) {

    /**
     * @constructor
     * @name BrowserMessagingStubFactory
     *
     * @param {Object} settings
     * @param {WebMessagingStub} settings.webMessagingStub an initialized sender that has the default window already set
     */
    function BrowserMessagingStubFactory(settings) {
        Typing.checkProperty(settings, "Object", "settings");
        Typing.checkProperty(
                settings.webMessagingStub,
                "WebMessagingStub",
                "settings.webMessagingStub");

        this._settings = settings;
    }

    /**
     * @name BrowserMessagingStubFactory#build
     * @function
     *
     * @param {BrowserMessagingAddress} address the address to generate a messaging stub for
     */
    BrowserMessagingStubFactory.prototype.build = function build(address) {
        return new BrowserMessagingStub({
            windowId : address.windowId,
            webMessagingStub : this._settings.webMessagingStub
        });
    };

            return BrowserMessagingStubFactory;

        }(Typing, BrowserMessagingStub));