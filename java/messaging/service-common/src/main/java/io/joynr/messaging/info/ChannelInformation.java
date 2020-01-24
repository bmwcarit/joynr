/*
 * #%L
 * joynr::java::messaging::service-common
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
package io.joynr.messaging.info;

import java.util.Optional;

/**
 * Information about a messaging channel that can be used by messaging clients.<br>
 * The purpose of this class is to have a more lightweight info class that can
 * be sent to clients without a lot of information overhead. The interface
 * specifies which data is relied on by clients. If a service wants to return
 * more detailed information this is done by adding getter methods.
 * 
 * @author christina.strobel
 * 
 */
public class ChannelInformation {
    public final String name;
    public final Integer resources;
    public final Integer cachedSize;

    public ChannelInformation(String name, Integer resources, Optional<Integer> cachedSize) {
        if (name == null) {
            name = "";
        }

        this.name = name;
        this.resources = resources;
        if (cachedSize.isPresent()) {
            this.cachedSize = cachedSize.get();
        } else {
            this.cachedSize = null;
        }
    }

    public String getName() {
        return name;
    }

    public Integer getResources() {
        return resources;
    }
}
