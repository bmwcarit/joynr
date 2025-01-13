/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2025 BMW Car IT GmbH
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
package io.joynr.sharedsubscription.test.clusteredapp.base;

import jakarta.inject.Inject;
import jakarta.ws.rs.Consumes;
import jakarta.ws.rs.GET;
import jakarta.ws.rs.POST;
import jakarta.ws.rs.Path;
import jakarta.ws.rs.Produces;
import jakarta.ws.rs.core.MediaType;

@Path("/callstats")
@Produces(MediaType.APPLICATION_JSON)
@Consumes(MediaType.APPLICATION_JSON)
public class CallStatisticsEndpoint {

    private CallStatistics callStatistics;

    @Inject
    public CallStatisticsEndpoint(CallStatistics callStatistics) {
        this.callStatistics = callStatistics;
    }

    @GET
    public String getCurrentNumberOfPingsReceived() {
        return String.valueOf(callStatistics.getCurrentCount());
    }

    @POST
    @Path("/reset")
    public void resetCount() {
        callStatistics.reset();
    }

}
