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
import Version from "../../generated/joynr/types/Version";
import { checkProperty, checkPropertyIfDefined } from "../util/UtilInternal";
import JoynrRuntimeException from "./JoynrRuntimeException";

class MethodInvocationException extends JoynrRuntimeException {
    /**
     * The provider version information
     */
    public providerVersion?: Version;
    public name = "MethodInvocationException";
    /**
     * Used for serialization.
     */
    public _typeName = "joynr.exceptions.MethodInvocationException";

    /**
     * Constructor of MethodInvocationException object used for reporting
     * error conditions when invoking a method (e.g. method does not
     * exist or no method with matching signature found etc.) that should
     * be transmitted back to consumer side.
     *
     * @param [settings] the settings object for the constructor call
     * @param [settings.providerVersion] the version of the provider
     *            which could not handle the method invocation
     * @param [settings.detailMessage] message containing details about the error
     */
    public constructor(settings: { providerVersion?: Version; detailMessage: string }) {
        super(settings);

        this.providerVersion = settings.providerVersion;
        if (settings) {
            checkProperty(settings, "Object", "settings");
            checkPropertyIfDefined(settings.providerVersion, "Version", "settings.providerVersion");
        }
    }

    public static _typeName = "joynr.exceptions.MethodInvocationException";
}

export = MethodInvocationException;
