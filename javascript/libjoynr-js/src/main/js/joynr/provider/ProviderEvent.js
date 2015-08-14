/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

define("joynr/provider/ProviderEvent", [], function() {

    /**
     * Constructor of ProviderEvent object that is used in the generation of provider objects
     *
     * @name ProviderEvent
     * @constructor
     *
     * @param {Provider} parent is the provider object that contains this attribute
     * @param {Object} [implementation] the definition of the event implementation
     * @param {String} eventName the name of the event
     * @param {String} eventCaps the capabilities of the event:
     *            [NOTIFY][READWRITE|READONLY|WRITEONLY], e.g. NOTIFYREADWRITE
     *            if the string contains 'NOTIFY' this event contains subscribe and unsubscribe
     *            if the string contains 'READ' or 'WRITE' this event has get and set
     */
    function ProviderEvent(parent, implementation, eventName) {
        if (!(this instanceof ProviderEvent)) {
            // in case someone calls constructor without new keyword (e.g. var c = Constructor({..}))
            return new ProviderEvent(implementation, eventName);
        }

        /**
         * if this attribute is changed the applicationshould call this function with the new value
         * which causes the a publication containing the new value to be sent to all subscribers.
         *
         * @name ProviderEvent#valueChanged
         * @function
         *
         * @param {?} value the new value of the attribute
         */
        this.valueChanged = function valueChanged(value) {
        //TODO: implement call all publish subscribers
        };

        return Object.freeze(this);
    }

    return ProviderEvent;

});