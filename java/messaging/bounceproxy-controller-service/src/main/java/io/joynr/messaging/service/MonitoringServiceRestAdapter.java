package io.joynr.messaging.service;

/*
 * #%L
 * joynr::java::messaging::bounceproxy-controller-service
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

import io.joynr.messaging.info.BounceProxyStatus;
import io.joynr.messaging.info.PerformanceMeasures;

import java.net.URI;
import java.util.List;
import java.util.Map.Entry;

import javax.ws.rs.GET;
import javax.ws.rs.HeaderParam;
import javax.ws.rs.POST;
import javax.ws.rs.PUT;
import javax.ws.rs.Path;
import javax.ws.rs.PathParam;
import javax.ws.rs.Produces;
import javax.ws.rs.QueryParam;
import javax.ws.rs.WebApplicationException;
import javax.ws.rs.core.Context;
import javax.ws.rs.core.GenericEntity;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.MultivaluedMap;
import javax.ws.rs.core.Response;
import javax.ws.rs.core.Response.Status;
import javax.ws.rs.core.UriInfo;

import com.google.inject.Inject;

/**
 * RESTful web service to control bounce proxy instances.
 * 
 * It offers methods for bounce proxies to report their status and also
 * performance measures. Based on these reports the bounce proxy controller will
 * be able to determine whether bounce proxy instances can handle more channels.
 * 
 * @author christina.strobel
 * 
 */
@Path("/controller")
public class MonitoringServiceRestAdapter {

    @Inject
    private MonitoringService monitoringService;

    /**
     * Returns a list of bounce proxies that are registered with the bounceproxy
     * controller.
     * 
     * @return
     */
    @GET
    @Produces("application/json")
    public GenericEntity<List<String>> getBounceProxies() {
        return new GenericEntity<List<String>>(monitoringService.getRegisteredBounceProxies()) {
        };
    }

    /**
     * Reports that a new bounce proxy instance has just been started. <br>
     * The BP instance is either completely new or is re-started after a
     * previous shutdown.
     * 
     * @param bpId
     * @param urlForCc
     * @param urlForBpc
     * @return
     */
    @POST
    @Produces({ MediaType.TEXT_PLAIN })
    public Response reportStartup(@QueryParam("bpid") String bpId,
                                  @HeaderParam("url4cc") String urlForCc,
                                  @HeaderParam("url4bpc") String urlForBpc) {

        if (!monitoringService.isRegistered(bpId)) {

            // first startup
            monitoringService.register(bpId, urlForCc, urlForBpc);
            return Response.created(URI.create(urlForBpc)).build();

        } else {

            // startup after a crash or a shutdown
            monitoringService.reset(bpId, urlForCc, urlForBpc);
            return Response.noContent().build();
        }
    }

    /**
     * Refreshes the status of a bounce proxy instance. This could either apply
     * to performance monitoring measures like active long polls or to the
     * status itself such as active or shutdown.
     * 
     * @param bpId
     * @param status
     * @param activeLongPollCount
     * @return
     */
    @PUT
    @Path("/{bpid: ([A-Z,a-z,0-9,_,\\-]+)(\\.)([A-Z,a-z,0-9,_,\\-]+)}")
    @Produces({ MediaType.TEXT_PLAIN })
    public Response reportStatus(@PathParam("bpid") String bpId,
                                 @QueryParam("status") BounceProxyStatusParam statusParam,
                                 @Context UriInfo ui) {

        // TODO This API should be rewritten to have two different methods for
        // this. Now either one set of parameters (status) or the other set of
        // parameters (performance measures) are used, but not all of them
        // together.

        if (statusParam != null && BounceProxyStatus.UNRESOLVED.equals(statusParam.getStatus())) {
            // bounce proxy sent an unknown status
            throw new WebApplicationException(Status.BAD_REQUEST);
        } else if (statusParam != null && BounceProxyStatus.SHUTDOWN.equals(statusParam.getStatus())) {
            monitoringService.updateStatus(bpId, statusParam.getStatus());
        } else {

            MultivaluedMap<String, String> queryParams = ui.getQueryParameters();
            PerformanceMeasures performanceMeasures = new PerformanceMeasures();

            for (Entry<String, List<String>> entry : queryParams.entrySet()) {

                // we only expect one value per key
                String key = entry.getKey();
                if (entry.getValue().size() != 1) {
                    // TODO for now, we ignore malformed performance measures
                } else {
                    // should be int value
                    String value = entry.getValue().get(0);

                    try {
                        int intVal = Integer.parseInt(value);
                        performanceMeasures.addMeasure(key, intVal);
                    } catch (NumberFormatException e) {
                        // TODO for now, we ignore malformed performance
                        // measures
                    }
                }
            }

            monitoringService.updatePerformanceMeasures(bpId, performanceMeasures);
        }

        return Response.noContent().build();
    }

}
