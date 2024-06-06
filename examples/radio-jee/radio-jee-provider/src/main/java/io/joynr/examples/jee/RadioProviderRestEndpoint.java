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
package io.joynr.examples.jee;

import jakarta.inject.Inject;
import jakarta.ws.rs.GET;
import jakarta.ws.rs.POST;
import jakarta.ws.rs.Path;
import jakarta.ws.rs.Produces;
import jakarta.ws.rs.core.MediaType;

@Path("/control")
@Produces(MediaType.APPLICATION_JSON)
public class RadioProviderRestEndpoint {

    private RadioService radioProviderBean;

    @Inject
    public RadioProviderRestEndpoint(RadioService radioProviderBean) {
        this.radioProviderBean = radioProviderBean;
    }

    @POST
    @Path("/fire-weak-signal")
    public void fireWeakSignal() {
        radioProviderBean.fireWeakSignal();
    }

    @GET
    @Path("/metrics")
    public String printMetrics() {
        return radioProviderBean.printMetrics();
    }
}
