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
package io.joynr.examples.customheaders;

import jakarta.ejb.ConcurrencyManagement;
import jakarta.ejb.ConcurrencyManagementType;
import jakarta.ejb.Singleton;
import jakarta.inject.Inject;

import io.joynr.jeeintegration.api.ServiceLocator;
import joynr.examples.customheaders.HeaderPingSync;

@Singleton
@ConcurrencyManagement(ConcurrencyManagementType.BEAN)
public class HeaderPingClient {

    @Inject
    private ServiceLocator serviceLocator;

    private HeaderPingSync headerPingProxy;

    public HeaderPingSync get() {
        if (headerPingProxy == null) {
            headerPingProxy = serviceLocator.get(HeaderPingSync.class, "io.joynr.examples.customheaders.jee.provider");
        }
        return headerPingProxy;
    }
}
