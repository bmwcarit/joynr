package io.joynr.messaging.service;

/*
 * #%L
 * joynr::java::messaging::channel-service
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

import io.joynr.messaging.info.ChannelStatus;

/**
 * Helper class to allow non-case sensitive parsing of {@link ChannelStatus}
 * parameters.
 * 
 * @author christina.strobel
 *
 */
public class ChannelStatusParam {

    private ChannelStatus status;

    public ChannelStatusParam(String status) {
        this.status = ChannelStatus.valueOf(status.toUpperCase());
    }

    public ChannelStatus getStatus() {
        return this.status;
    }
}
