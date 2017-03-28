package io.joynr.communications.exceptions;

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

import io.joynr.messaging.datatypes.JoynrErrorCode;
import io.joynr.messaging.datatypes.JoynrMessagingError;

import javax.ws.rs.WebApplicationException;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;
import javax.ws.rs.core.Response.Status;

/**
 * This runtime exception can be thrown by servlets to indicate a specific error
 * to a client.<br>
 * It produces a HTTP error response and contains an error code that can be
 * processed by the client.
 */
public class JoynrHttpException extends WebApplicationException {

    private static final long serialVersionUID = -8459325454159353237L;
    private String message = "";

    public JoynrHttpException() {
        super();

    }

    public JoynrHttpException(Status status, JoynrErrorCode errorCode) {
        this(status.getStatusCode(), errorCode.getCode(), errorCode.getDescription());
    }

    public JoynrHttpException(Status status, JoynrErrorCode errorCode, String message) {
        this(status.getStatusCode(), errorCode.getCode(), errorCode.getDescription() + ": " + message);
    }

    public JoynrHttpException(int status, int code, String message) {
        super(Response.serverError()
                      .status(status)
                      .type(MediaType.APPLICATION_JSON)
                      .entity(new JoynrMessagingError(code, message))
                      .build());

        this.message = message;
    }

    @Override
    public String getMessage() {
        return " reason: " + message;
    }

}
