package joynr.exceptions;

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

import io.joynr.exceptions.JoynrRuntimeException;

/**
 * Joynr exception to report errors at the provider if no error enums are defined
 * in the corresponding Franca model file. It will also be used to wrap an transmit
 * unexpected exceptions which are thrown by the provider.
 */
public class ProviderRuntimeException extends JoynrRuntimeException {
    private static final long serialVersionUID = 1L;

    /**
     * Constructor for a ProviderRuntimeException with detail message.
     *
     * @param message further description of the reported error
     */
    public ProviderRuntimeException(String message) {
        super(message);
    }
}
