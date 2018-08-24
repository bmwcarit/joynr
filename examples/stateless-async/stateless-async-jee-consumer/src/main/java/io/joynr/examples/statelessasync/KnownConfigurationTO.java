/*-
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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
package io.joynr.examples.statelessasync;

public class KnownConfigurationTO {
    private String messageId;
    private String vehicleConfigurationId;
    private boolean successfullyAdded;

    public KnownConfigurationTO() {
    }

    public KnownConfigurationTO(String messageId, String vehicleConfigurationId, boolean successfullyAdded) {
        this.messageId = messageId;
        this.vehicleConfigurationId = vehicleConfigurationId;
        this.successfullyAdded = successfullyAdded;
    }

    public String getMessageId() {
        return messageId;
    }

    public void setMessageId(String messageId) {
        this.messageId = messageId;
    }

    public String getVehicleConfigurationId() {
        return vehicleConfigurationId;
    }

    public void setVehicleConfigurationId(String vehicleConfigurationId) {
        this.vehicleConfigurationId = vehicleConfigurationId;
    }

    public boolean isSuccessfullyAdded() {
        return successfullyAdded;
    }

    public void setSuccessfullyAdded(boolean successfullyAdded) {
        this.successfullyAdded = successfullyAdded;
    }
}
