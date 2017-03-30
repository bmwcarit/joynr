package io.joynr.messaging.bounceproxy.controller.exception;

/*
 * #%L
 * joynr::java::messaging::bounceproxy::bounceproxy-controller
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

/**
 * Exception that indicates that a bounce proxy is behaving in a way that does
 * not conform with the specification of the protocol between bounce proxy and
 * bounce proxy controller.
 * 
 * @author christina.strobel
 * 
 */
public class JoynrProtocolException extends Exception {

    private static final long serialVersionUID = 1156488423750044818L;

    public JoynrProtocolException(String message) {
        super(message);
    }

    public JoynrProtocolException(String message, Throwable t) {
        super(message, t);
    }
}
