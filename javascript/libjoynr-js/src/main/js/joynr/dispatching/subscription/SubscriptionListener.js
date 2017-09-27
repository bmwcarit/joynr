/*jslint node: true */

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
module.exports = (function() {

    /**
     * @name SubscriptionListener
     * @constructor
     *
     * @param {Function}
     *            settings.onReceive The function to call when a publication is received
     * @param {Function}
     *            settings.onError The function to call if no publication is received in the given alert interval
     *            or if some other error occurs. The error is passed as parameter.
     */
    function SubscriptionListener(settings) {
        if (!(this instanceof SubscriptionListener)) {
            // in case someone calls constructor without new keyword (e.g. var c = Constructor({..}))
            return new SubscriptionListener(settings);
        }

        /**
         * Is called if subscription request has been successfully delivered to the provider
         * @name SubscriptionListener#onSubscribed
         */
        this.onSubscribed = settings.onSubscribed;

        /**
         * Is called if publication is received
         * @name SubscriptionListener#onReceive
         */
        this.onReceive = settings.onReceive;

        /**
         * Is called if publication is missed or an error occurs
         * @name SubscriptionListener#onError
         */
        this.onError = settings.onError;

    }

    return SubscriptionListener;

}());