/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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

export interface ReplySettings {
    response?: any[];
    error?: any;
    requestReplyId: string;
}

export interface Reply extends ReplySettings {
    _typeName: "joynr.Reply";
}

/**
 * @param settings.requestReplyId
 * @param [settings.response] the response may be undefined
 * @param [settings.error] The exception object in case of request failure
 */
export function create(settings: ReplySettings): Reply {
    // must contain exactly one of the two alternatives
    if (!settings.response && !settings.error) {
        throw new Error("Reply object does neither contain response nor error");
    }
    if (settings.error && Array.isArray(settings.response) && settings.response.length > 0) {
        throw new Error("Reply object contains both response and error");
    }
    (settings as Reply)._typeName = "joynr.Reply";
    return settings as Reply;
}
