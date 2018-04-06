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
var Typing = require("../util/Typing");
var Util = require("../util/UtilInternal");
var LoggingManager = require("../system/LoggingManager");

var log = LoggingManager.getLogger("joynr/messaging/MessagingSkeletonFactory");
/**
 * @name MessagingSkeletonFactory
 * @constructor
 *
 */
function MessagingSkeletonFactory() {
    this._messagingSkeletons = undefined;
}

MessagingSkeletonFactory.prototype.setSkeletons = function setSkeletons(newMessagingSkeletons) {
    this._messagingSkeletons = newMessagingSkeletons;
};

/**
 * @name MessagingSkeletonFactory#getSkeleton
 * @function
 *
 * return {MessagingSkeleton} the skeleton matching the address
 */
MessagingSkeletonFactory.prototype.getSkeleton = function getSkeleton(address) {
    var className = address._typeName;
    var skeleton = this._messagingSkeletons[className];

    if (Util.checkNullUndefined(skeleton)) {
        var errorMsg =
            'Could not find a messaging skeleton for "' +
            className +
            '" within messagingSkeletons [' +
            Object.keys(this._messagingSkeletons).join(",") +
            "]";
        log.debug(errorMsg);
        throw new Error(errorMsg);
    }
    return skeleton;
};

module.exports = MessagingSkeletonFactory;
