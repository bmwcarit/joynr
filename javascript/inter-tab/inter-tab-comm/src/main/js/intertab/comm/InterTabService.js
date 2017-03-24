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

define("intertab/comm/InterTabService", [ ], function() {
    /**
     * @param settings - parameter required for the proper initialization of the InterTabService
     * @param settings.interTabReceiver - add the InterTabService as event listener for messages
     * @param settings.interTabSender - abstraction to send plain messages via the Messaging API
     * @param settings.resolveWindow - window resolver function based on the provided window ID
     * @param settings.onError - error callback
     */
    function InterTabService(settings) {

        settings.interTabReceiver.registerReceive(function(event) {
            var message = event.data.message;
            var windowIdTo = event.data.windowId; // retrieve target windowId
            var targetWindow = settings.resolveWindow(windowIdTo); // get window for target windowId
            if (targetWindow) {
                try {
                    settings.interTabSender.postMessage({
                        message: message,
                        targetWindow : targetWindow
                    });
                } catch(e) {
                    settings.onError("[ERROR] InterTabService._receive: post message to target window with id \"" + windowIdTo + "\" failed. Error: " + e);
                }
            } else {
                if (settings.onError){
                    settings.onError("[ERROR] InterTabService._receive: no route for window with windowId \"" + windowIdTo + "\"");
                }
            }
        });
    }
    return InterTabService;
});