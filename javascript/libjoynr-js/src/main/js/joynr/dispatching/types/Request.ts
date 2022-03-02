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

import { nanoid } from "nanoid";

const rrBase = nanoid();
let rrIndex = 0;

interface RequestParams {
    requestReplyId?: string;
    methodName: string;
    paramDatatypes?: string[];
    params?: any[];
}

export interface Request {
    requestReplyId: string;
    methodName: string;
    paramDatatypes: string[];
    params?: any[];
    _typeName: string;
}

/**
 * @param settings.requestReplyId
 * @param settings.methodName
 * @param [settings.paramDatatypes] parameter datatypes
 * @param [settings.params] parameters
 */
export function create(settings: RequestParams): Request {
    settings.requestReplyId = settings.requestReplyId || `${rrBase}_${rrIndex++}`;

    if (!settings.paramDatatypes) {
        settings.paramDatatypes = [];
    }

    /**
     * The joynr type name
     *
     * @name Request#_typeName
     * @type String
     */
    (settings as Request)._typeName = "joynr.Request";
    return settings as Request;
}
