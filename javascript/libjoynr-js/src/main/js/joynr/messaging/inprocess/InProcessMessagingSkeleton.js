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

define("joynr/messaging/inprocess/InProcessMessagingSkeleton", [], function() {

    /**
     * @name InProcessMessagingSkeleton
     * @constructor
     */
    function InProcessMessagingSkeleton() {
        var onReceive;

        /**
         * @name InProcessMessagingSkeleton#receiveMessage
         * @function
         *
         * @param {JoynrMessage} joynrMessage
         * @returns {Object} A+ promise object
         */
        this.receiveMessage = function receiveMessage(joynrMessage) {
            return onReceive(joynrMessage);
        };

        /**
         * A setter for the callback function that will receive the incoming messages
         *
         * @name InProcessMessagingSkeleton#registerListener
         * @function
         *
         * @param {Function} newOnReceive the function that is called with the incoming JoynrMessage
         */
        this.registerListener = function registerListener(newOnReceive) {
            onReceive = newOnReceive;
        };
    }

    return InProcessMessagingSkeleton;

});