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
package io.joynr.systemintegrationtest.jee;

import com.google.inject.Inject;
import jakarta.ws.rs.GET;
import jakarta.ws.rs.Path;
import jakarta.ws.rs.Produces;
import jakarta.ws.rs.QueryParam;
import jakarta.ws.rs.core.MediaType;

import joynr.test.SitControllerSync;

/**
 * Exposes a REST endpoint with which the consumer side joynr system integration tests can be triggered.
 */
@Path("/sit-controller")
@Produces(MediaType.APPLICATION_JSON)
public class ConsumerRestEndpoint {

    private SitControllerSync sitControllerProviderBean;

    @Inject
    public ConsumerRestEndpoint(SitControllerSync sitControllerProviderBean) {
        this.sitControllerProviderBean = sitControllerProviderBean;
    }

    @GET
    @Path("/ping")
    public String ping() {
        return sitControllerProviderBean.ping();
    }

    @GET
    @Path("/test")
    public String triggerTests(@QueryParam("domains") String domains,
                               @QueryParam("expectFailure") Boolean expectFailure) {
        return sitControllerProviderBean.triggerTests(domains, expectFailure);
    }

}
