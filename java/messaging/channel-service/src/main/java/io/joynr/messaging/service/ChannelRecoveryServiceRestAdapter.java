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
package io.joynr.messaging.service;

import static io.joynr.messaging.datatypes.JoynrMessagingErrorCode.JOYNRMESSAGINGERROR_CHANNELNOTSET;

import java.util.Optional;

import javax.servlet.http.HttpServletRequest;
import javax.ws.rs.HeaderParam;
import javax.ws.rs.PUT;
import javax.ws.rs.Path;
import javax.ws.rs.PathParam;
import javax.ws.rs.QueryParam;
import javax.ws.rs.WebApplicationException;
import javax.ws.rs.core.Context;
import javax.ws.rs.core.Response.Status;
import javax.ws.rs.core.Response;

import com.google.inject.Inject;

import io.joynr.communications.exceptions.JoynrHttpException;
import io.joynr.messaging.info.Channel;
import io.joynr.messaging.info.ChannelStatus;

/**
 * Channel service extension for scenarios in which a channel error is reported
 * to a controller that then tries to recover that channel.
 *
 * @author christina.strobel
 *
 */
@Path("channels")
public class ChannelRecoveryServiceRestAdapter {

    @Context
    HttpServletRequest request;

    @Inject
    private ChannelRecoveryService channelService;

    @Inject
    private ChannelErrorNotifier errorNotifier;

    /**
     * Query to recover a channel that previously rejected messages or was
     * unreachable. <br>
     * The interface has been introduced for controlled bounce proxies.
     *
     * @param ccid
     *   identifier of the channel to recover
     * @param bpId
     *   identifier of the bounce proxy handling this channel
     * @param statusParam
     *   the channel status
     * @param atmosphereTrackingId
     *   the atmosphere tracking id
     * @return
     *   response builder object for either empty response
     *   ((a) if channel recovered on previously assigned bounce proxy or
     *   (b) bounce proxy reachable by bounce proxy controller,
     *   but possibly no by cluster controllers),
     *   with ok status ((c) if channel was moved to another bounce proxy),
     *   or for a created resource ((d) in case bounce proxy controller lost data).
     */
    @PUT
    @Path("/{ccid: [A-Z,a-z,0-9,_,\\-,\\.]+}")
    public Response recoverChannel(@PathParam("ccid") String ccid,
                                   @QueryParam("bp") String bpId,
                                   @QueryParam("status") ChannelStatusParam statusParam,
                                   @HeaderParam(ChannelServiceConstants.X_ATMOSPHERE_TRACKING_ID) String atmosphereTrackingId) {

        if (ccid == null || ccid.isEmpty())
            throw new JoynrHttpException(Status.BAD_REQUEST, JOYNRMESSAGINGERROR_CHANNELNOTSET);

        try {

            Optional<Channel> channel = channelService.getChannel(ccid);

            if (!channel.isPresent()) {

                // bounce proxy controller lost data
                Channel newChannel = channelService.createChannel(ccid, atmosphereTrackingId);
                return Response.created(newChannel.getLocation())
                               .header("bp", newChannel.getBounceProxy().getId())
                               .build();

            } else {

                if (!bpId.equals(channel.get().getBounceProxy().getId())) {
                    // channel was moved to another bounce proxy
                    return Response.ok()
                                   .header("Location", channel.get().getLocation().toString())
                                   .header("bp", channel.get().getBounceProxy().getId())
                                   .build();
                } else {
                    // bounce proxy exists, but was unreachable for cluster
                    // controllers

                    if (statusParam == null) {
                        // we need a status to process how it is going on
                        throw new JoynrHttpException(Status.BAD_REQUEST.getStatusCode(), 0, "No channel status set");
                    } else {
                        ChannelStatus status = statusParam.getStatus();

                        if (status.equals(ChannelStatus.REJECTING_LONG_POLLS)) {

                            // recover the channel on the previously assigned
                            // bounce proxy
                            channelService.recoverChannel(ccid, atmosphereTrackingId);

                            return Response.noContent().build();

                        } else if (status.equals(ChannelStatus.UNREACHABLE)) {

                            if (channelService.isBounceProxyForChannelResponding(ccid)) {

                                // bounce proxy reachable by bounce proxy
                                // controller, but possibly not by cluster
                                // controllers
                                errorNotifier.alertBounceProxyUnreachable(ccid,
                                                                          bpId,
                                                                          request.getRemoteAddr(),
                                                                          "Bounce Proxy unreachable for Cluster Controller");

                                return Response.noContent().build();

                            } else {

                                // bounce proxy dead
                                errorNotifier.alertBounceProxyUnreachable(ccid,
                                                                          bpId,
                                                                          request.getLocalAddr(),
                                                                          "Bounce Proxy unreachable for Channel Service");

                                // TODO
                                // channelServiceDelegate.markBounceProxyAsUnreachable(bpId);

                                // create new channel on different bounce proxy
                                Channel newChannel = channelService.createChannel(ccid, atmosphereTrackingId);
                                return Response.created(newChannel.getLocation())
                                               .header("bp", newChannel.getBounceProxy().getId())
                                               .build();
                            }
                        } else {
                            throw new JoynrHttpException(Status.BAD_REQUEST.getStatusCode(),
                                                         0,
                                                         "Unknown channel status '" + status + "'");
                        }
                    }
                }
            }

        } catch (Exception e) {
            throw new WebApplicationException(e, Status.INTERNAL_SERVER_ERROR);
        }

    }
}
