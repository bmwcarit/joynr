package io.joynr.messaging.service;

/*
 * #%L
 * joynr::java::messaging::bounceproxy-controller-service
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

import javax.ws.rs.DELETE;
import javax.ws.rs.Path;
import javax.ws.rs.PathParam;
import javax.ws.rs.core.Response;

import com.google.inject.Inject;

/**
 * RESTful web service interface definition for bounce proxy migration service.
 * 
 * @author christina.strobel
 *
 */
@Path("/migration")
public class MigrationServiceRestAdapter {

    @Inject
    private MigrationService migrationService;

    /**
     * Migrates all bounce proxies of one cluster to a different cluster.
     * 
     * @param clusterId the identifier of the cluster to migrate
     * @return response with status 202 (accepted)
     */
    @DELETE
    @Path("/clusters/{clusterid: ([A-Z,a-z,0-9,_,\\-]+)}")
    public Response migrateCluster(@PathParam("clusterid") final String clusterId) {

        // as this is a long running task, this has to be asynchronous
        migrationService.startClusterMigration(clusterId);

        return Response.status(202 /* Accepted */).build();
    }

    /**
     * TODO placeholder for future implementation
     * 
     * Migrates a single bounce proxy to a different cluster.
     * 
     * @param bpId bounce proxy id
     * @return response with status 501 (not implemented)
     */
    @DELETE
    @Path("/bps/{bpid}")
    public Response migrateBounceProxy(@PathParam("bpid") String bpId) {
        return Response.status(501 /* Not Implemented */).build();
    }

    /**
     * TODO placeholder for future implementation
     * 
     * Migrates a single channel to a different bounce proxy.
     * @param ccid cluster controller id
     * @return response with status 501 (not implemented)
     */
    @DELETE
    @Path("/channels/{ccid}")
    public Response migrateChannel(@PathParam("ccid") String ccid) {
        return Response.status(501 /* Not Implemented */).build();
    }
}
