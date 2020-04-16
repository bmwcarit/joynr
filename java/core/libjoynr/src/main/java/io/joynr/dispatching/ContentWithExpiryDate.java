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
package io.joynr.dispatching;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.common.ExpiryDate;

public class ContentWithExpiryDate<T> {
    private static final Logger logger = LoggerFactory.getLogger(ContentWithExpiryDate.class);
    private T content;
    private ExpiryDate expiryDate;

    public ContentWithExpiryDate(T content, ExpiryDate roundTripExpirationDate) {
        this.content = content;
        this.expiryDate = roundTripExpirationDate;

    }

    public T getContent() {
        return content;
    }

    boolean isExpired() {
        return (System.currentTimeMillis() > expiryDate.getValue());
    }
}
