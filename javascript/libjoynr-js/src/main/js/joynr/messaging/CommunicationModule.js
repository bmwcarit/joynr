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
var Promise = require('../../global/Promise');
var XMLHttpRequestDependency = require('../../global/XMLHttpRequestNode');
var atmosphereDependency = require('../../lib/atmosphereNode');
var LongTimer = require('../util/LongTimer');

/**
 * Constructor of CommunicationModule object that is used to stsub communication with the outer world
 *
 * @constructor
 * @name CommunicationModule
 *
 * @returns {CommunicationModule} the communication module object
 */
function CommunicationModule() {
    if (!(this instanceof CommunicationModule)) {
        // in case someone calls constructor without new keyword (e.g. var c = Constructor({..}))
        return new CommunicationModule();
    }

    /**
     * The atmosphere object
     *
     * @name CommunicationModule#atmosphere
     * @type Object
     */
    this.atmosphere = atmosphereDependency;

}

/**
 * Submitting an async http xml request
 *
 * @name CommunicationModule#createXMLHTTPRequest
 * @function
 *
 * @param {Object} parameters - an object containing the required parameters
 * @param {String} parameters.data - the data to be sent
 * @param {Array} parameters.headers - an array of headers to be sent
 * @param {Number} parameters.timeout - the timeout for the request
 * @param {String} parameters.type - the type of request
 * @param {String} parameters.url - the location
 */
CommunicationModule.prototype.createXMLHTTPRequest = function(parameters) {
    return new Promise(function(fulfill, reject) {
        var xhr = new XMLHttpRequestDependency();
        var async = true;

        xhr.open(parameters.type, parameters.url, async);
        //xhr.setRequestHeader('Content-type', 'application/json; charset=utf-8');
        xhr.setRequestHeader("Content-type", "text/plain"); // workaround for TODO # 1174
        if (parameters.headers) {
            var headerEntry;
            for (headerEntry in parameters.headers) {
                if (parameters.headers.hasOwnProperty(headerEntry)) {
                    xhr.setRequestHeader(headerEntry, parameters.headers[headerEntry]);
                }
            }
        }

        function postTimeoutHandler() {
            xhr.onreadystatechange = undefined;
            xhr.abort();
            reject(new Error('xhr, "request timed out after " + parameters.timeout + "ms."'));
        }

        var postTimeout = LongTimer.setTimeout(postTimeoutHandler, parameters.timeout);

        function xhrOnReadyStateChange() {
            /*
             * readyState   Holds the status of the XMLHttpRequest. Changes from 0 to 4:
             * 0: request not initialized
             * 1: server connection established
             * 2: request received
             * 3: processing request
             * 4: request finished and response is ready
             */
            var isRequestFinishedAndResponseReady = xhr.readyState === 4;
            if (isRequestFinishedAndResponseReady) {
                LongTimer.clearTimeout(postTimeout);
                // Unless stated otherwise, the response status will be 200
                if ((xhr.status >= 200 && xhr.status < 300) || xhr.status === 304) {
                    fulfill(xhr);
                } else {
                    reject(new Error("xhr, xhr.status ? 'error' : 'abort'"));
                }
            }
        }
        xhr.onreadystatechange = xhrOnReadyStateChange;
        xhr.send(parameters.data);
    });
};

module.exports = CommunicationModule;
