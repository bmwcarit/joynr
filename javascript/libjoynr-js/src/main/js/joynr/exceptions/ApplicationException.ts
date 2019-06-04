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
import { checkProperty } from "../util/UtilInternal";
import JoynrException from "./JoynrException";
const defaultMessage = "This is an application exception.";

interface ErrorEnum {
    name: string;
    value: string;
}

class ApplicationException extends JoynrException {
    public error: any;
    public name: string;

    /**
     * Used for serialization.
     */
    public _typeName: string;
    /**
     * Constructor of ApplicationException object used for reporting
     * error conditions from method implementations. The settings.error
     * object must be filled with _typeName and name as serialization
     * of an enum object of the matching error enum type defined in
     * Franca.
     *
     * @param [settings] the settings object for the constructor call
     * @param settings.error the error enum to be reported
     * @param [settings.detailMessage] message containing details about the error
     */
    public constructor(settings: { detailMessage: string; error?: ErrorEnum }) {
        settings.detailMessage = settings.detailMessage || defaultMessage;
        super(settings);

        this.name = "ApplicationException";
        this._typeName = "joynr.exceptions.ApplicationException";
        this.error = settings.error;

        if (settings.error) {
            checkProperty(settings.error.name, "String", "settings.error.name");
            checkProperty(settings.error.value, ["String", "Number"], "settings.error.value");
        }
    }

    public static _typeName = "joynr.exceptions.ApplicationException";
}

export = ApplicationException;
