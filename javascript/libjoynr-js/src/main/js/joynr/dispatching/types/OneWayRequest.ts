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

interface OneWayRequestParams {
    methodName: string;
    paramDatatypes?: string[];
    params?: any[];
}

export type OneWayRequest = OneWayRequestParams & { _typeName: string };

/**
 * @param settings.methodName
 * @param settings.paramDatatypes parameter datatypes
 * @param settings.params parameters
 */
export function create(settings: OneWayRequestParams): OneWayRequest {
    (settings as OneWayRequest)._typeName = "joynr.OneWayRequest";
    return settings as any;
}
