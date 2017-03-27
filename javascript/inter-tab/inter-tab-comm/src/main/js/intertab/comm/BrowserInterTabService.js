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

define("intertab/comm/BrowserInterTabService",
    [
        "intertab/comm/WebMessageReceiver",
        "intertab/comm/WebMessageSender",
        "intertab/comm/InterTabService"
    ],
    function(
     WebMessageReceiver,
     WebMessageSender,
     InterTabService) {
    /**
     * @param settings - parameter required for the proper initialization of the BrowserInterTabService
     * @param settings.resolveWindow - window resolver function based on the provided window ID
     * @param settings.onError - error callback
     */
    function BrowserInterTabService(settings) {
        if (!(this instanceof BrowserInterTabService)) {
            // in case someone calls constructor without new keyword (e.g. var c
            // = Constructor({..}))
            return new BrowserInterTabService(settings);
        }

        var interTabService = new InterTabService({
            interTabReceiver : new WebMessageReceiver({
                window: window
            }),
            interTabSender : new WebMessageSender(),
            resolveWindow : settings.resolveWindow,
            onError : settings.onError
        });
    }

    return BrowserInterTabService;
});
