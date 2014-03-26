package io.joynr.exceptions;

import java.text.DateFormat;
import java.text.SimpleDateFormat;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

public class JoynrTimeoutException extends JoynrException {
    private static final long serialVersionUID = 1L;
    private static final ThreadLocal<DateFormat> DateFormatter = new ThreadLocal<DateFormat>() {
        @Override
        protected DateFormat initialValue() {
            return new SimpleDateFormat("dd/MM HH:mm:ss:sss");
        }
    };

    private long expiryDate;

    public JoynrTimeoutException(long expiryDate) {
        super("ttl expired on: " + DateFormatter.get().format(expiryDate));
        this.expiryDate = expiryDate;

    }

    public long getExpiryDate() {
        return expiryDate;
    }

}
