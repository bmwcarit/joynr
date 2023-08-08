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
package io.joynr.guice;

import java.util.Locale;
import java.util.Properties;

public class LowerCaseProperties extends Properties {
    private static final long serialVersionUID = 4043745541472262866L;

    public LowerCaseProperties() {
    }

    public LowerCaseProperties(Properties loadProperties) {
        this.putAll(loadProperties);
    }

    @Override
    public synchronized Object put(Object key, Object value) {
        if (key instanceof String) {
            return super.put(((String) key).toLowerCase(Locale.ROOT), value);
        }
        return super.put(key, value);
    }
}
