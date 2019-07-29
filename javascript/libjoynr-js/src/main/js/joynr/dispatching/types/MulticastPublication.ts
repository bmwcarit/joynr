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

export interface MulticastPublicationSettings {
    multicastId: string;
    response?: any;
    error?: Record<string, any>;
}

export interface MulticastPublication extends MulticastPublicationSettings {
    _typeName: "joynr.MulticastPublication";
}

/**
 * @name MulticastPublication
 * @constructor
 *
 * @param settings.multicastId
 * @param settings.response
 * @param settings.error The exception object in case of publication failure
 */
export function create(settings: MulticastPublicationSettings): MulticastPublication {
    /**
     * The joynr type name
     *
     * @name MulticastPublication#_typeName
     * @type String
     */
    (settings as MulticastPublication)._typeName = "joynr.MulticastPublication";
    return settings as MulticastPublication;
}
