/*global process: true*/

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

define("joynr/security/PlatformSecurityManager", [], function() {

    /**
     * @name PlatformSecurityManager
     * @constructor
     * @variation 2
     *
     * @param {String}
     *            messageType the message type as defined by
     *            [JoynrMessage.JOYNRMESSAGE_TYPE_*]{@link JoynrMessage}
     */
    function PlatformSecurityManager() {
        /**
         * @name PlatformSecurityManager(2)#getCurrentProcessUserId
         * @function
         *
         * @returns {String} the user ID that executes node
         */
        this.getCurrentProcessUserId = function getCurrentProcessUserId() {
            // Remark: Faking the user name by setting this environment variable
            // is not a security threat, since the process can only access the
            // private key of the real user. This key is used to sign the message.
            // On the receiving side joynr will try to verify the message signature
            // with the public key of the faked user which will fail.
            return process.env.USER;
        };
    }

    return PlatformSecurityManager;

});