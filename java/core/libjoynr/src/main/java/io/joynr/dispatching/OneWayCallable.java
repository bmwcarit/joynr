package io.joynr.dispatching;

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

import java.util.concurrent.Callable;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.common.ExpiryDate;

/**
 * Wraps a callable which contains the code to execute a one-way request with
 * information about when the request expires as well as a description used
 * for logging information about calling the request, so that the person using
 * the log knows which request is meant.
 */
public class OneWayCallable extends ContentWithExpiryDate<Callable<Void>> {

    private static final Logger logger = LoggerFactory.getLogger(OneWayCallable.class);

    private String requestDescriptor;

    public OneWayCallable(Callable<Void> requestHandler, ExpiryDate expiryDate, String requestDescriptor) {
        super(requestHandler, expiryDate);
        this.requestDescriptor = requestDescriptor;
    }

    /**
     * Executes the request by invoking the wrapped callable unless the call
     * has already {@link #isExpired() expired}. If the call is expired or
     * throws an exception while being executed, an appropriate message
     * is logged out, including the {@link #requestDescriptor request descriptor}
     * this instance was created with.
     */
    public void call() {
        if (isExpired()) {
            logger.warn("One-way request {} is expired. Not executing.", requestDescriptor);
        } else {
            try {
                getContent().call();
            } catch (Exception e) {
                logger.error("Error while executing one-way request {}.", requestDescriptor, e);
            }
        }
    }

}
