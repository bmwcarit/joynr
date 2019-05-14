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
 * @constructor
 * @name InProcessMessagingStub
 *
 * @param {InProcessMessagingSkeleton} inProcessMessagingSkeleton the skeleton to send the joynr messages to
 */
function InProcessMessagingStub(inProcessMessagingSkeleton) {
    this._inProcessMessagingSkeleton = inProcessMessagingSkeleton;
}

/**
 * @name InProcessMessagingStub#transmit
 * @function
 *
 * @param {JoynrMessage} message the message to transmit
 * @returns {Object} A+ promise object
 */
InProcessMessagingStub.prototype.transmit = function transmit(joynrMessage) {
    return this._inProcessMessagingSkeleton.receiveMessage(joynrMessage);
};
module.exports = InProcessMessagingStub;
