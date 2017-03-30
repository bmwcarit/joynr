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

define("joynr/dispatching/types/SubscriptionStop", [
    "joynr/util/UtilInternal",
    "joynr/util/Typing"
], function(Util, Typing) {

    /**
     * @name SubscriptionStop
     * @constructor
     *
     * @param settings
     *            {Object}
     * @param settings.subscriptionId
     *            {String}
     */
    function SubscriptionStop(settings) {
        Typing.checkProperty(settings.subscriptionId, "String", "settings.subscriptionId");

        /**
         * @name SubscriptionStop#subscriptionId
         * @type String
         */
        Util.extend(this, settings);

        /**
         * The joynr type name
         *
         * @name SubscriptionStop#_typeName
         * @type String
         */
        Typing.augmentTypeName(this, "joynr");

        return Object.freeze(this);
    }

    return SubscriptionStop;

});
