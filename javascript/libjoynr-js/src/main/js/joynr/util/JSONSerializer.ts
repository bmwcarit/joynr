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
import * as Typing from "./Typing";

/**
 * This function wraps the JSON.stringify call, by altering the stringification process
 * for joynr types
 * @param value to be stringified
 * @returns the value in JSON notation
 */
export function stringify(value: any): string {
    return JSON.stringify(value, (_key, src: any): string => (Typing.isEnumType(src) ? src.name : src));
}

/**
 * Utility function to either call JSON.stringify directly or with stringify replacement function
 * @param value to be stringified
 * @param omitReplacement
 * @returns the value in JSON notation
 */
export function stringifyOptional(value: any, omitReplacement: boolean): string {
    return omitReplacement ? JSON.stringify(value) : stringify(value);
}
