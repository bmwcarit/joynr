package io.joynr.bounceproxy.info;

import javax.annotation.CheckForNull;

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

public class ChannelInformation {
    public final String name;
    public final Integer resources;
    public final Integer cachedSize;

    public ChannelInformation(String name, Integer resources, @CheckForNull Integer cachedSize) {
        if (name == null) {
            name = "";
        }

        this.name = name;
        this.resources = resources;
        this.cachedSize = cachedSize;
    }

    public String getName() {
        return name;
    }

    public Integer getResources() {
        return resources;
    }
}
