/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2024 BMW Car IT GmbH
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

import static java.lang.String.valueOf;

import java.util.ArrayList;
import java.util.List;

import jakarta.inject.Inject;
import jakarta.ws.rs.GET;
import jakarta.ws.rs.Path;
import jakarta.ws.rs.PathParam;

import joynr.io.joynr.sharedsubscriptions.test.PingServiceSync;

@Path("/ping")
public class PingEndpoint {

    private PingServiceSync pingProviderBean;

    @Inject
    public PingEndpoint(PingServiceSync pingProviderBean) {
        this.pingProviderBean = pingProviderBean;
    }

    @GET
    public String triggerPing() {
        return pingProviderBean.ping();
    }

    @GET
    @Path("/{numberOfPings}")
    public String triggerMultiplePings(@PathParam("numberOfPings") int numberOfPings) {
        List<String> answers = new ArrayList<>();
        for (int count = 0; count < numberOfPings; count++) {
            try {
                answers.add(pingProviderBean.ping() + "\n");
            } catch (RuntimeException e) {
                answers.add(valueOf(e) + "\n");
            }
        }
        return String.valueOf(answers);
    }

}
