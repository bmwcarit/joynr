/*
 * #%L
 * joynr::java::messaging::messaging-service
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
import static io.joynr.messaging.datatypes.JoynrMessagingErrorCode.JOYNRMESSAGINGERROR_DESERIALIZATIONFAILED;
import static io.joynr.messaging.datatypes.JoynrMessagingErrorCode.JOYNRMESSAGINGERROR_EXPIRYDATEEXPIRED;
import static io.joynr.messaging.datatypes.JoynrMessagingErrorCode.JOYNRMESSAGINGERROR_EXPIRYDATENOTSET;

import java.net.URI;

import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.ws.rs.POST;
import javax.ws.rs.Path;
import javax.ws.rs.PathParam;
import javax.ws.rs.Produces;
import javax.ws.rs.WebApplicationException;
import javax.ws.rs.core.Context;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response.Status;
import javax.ws.rs.core.Response;
import javax.ws.rs.core.UriInfo;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;

import io.joynr.communications.exceptions.JoynrHttpException;
import io.joynr.messaging.system.TimestampProvider;
import io.joynr.smrf.EncodingException;
import io.joynr.smrf.UnsuppportedVersionException;
import joynr.ImmutableMessage;

@Path("/channels/{ccid: [A-Z,a-z,0-9,_,\\-,\\.]+}/message")
/**
 * MessagingService is used by the messaging service of a cluster controller
 * to register for messages from other cluster controllers
 *
 * The following characters are allowed in the id
 * - upper and lower case characters
 * - numbers
 * - underscore (_) hyphen (-) and dot (.)
 */
public class MessagingServiceRestAdapter {

    private static final Logger log = LoggerFactory.getLogger(MessagingServiceRestAdapter.class);

    @Inject
    private MessagingService messagingService;

    @Inject
    private TimestampProvider timestampProvider;

    @Context
    UriInfo ui;

    @Context
    HttpServletRequest request;

    @Context
    HttpServletResponse response;

    /**
     * Send a message.
     *
     * @param ccid
     *            channel id of the receiver.
     * @param serializedMessage
     *            the content being sent (serialized SMRF message).
     * @return a location for querying the message status
     */
    @POST
    @Produces({ MediaType.APPLICATION_OCTET_STREAM })
    public Response postMessage(@PathParam("ccid") String ccid, byte[] serializedMessage) {
        ImmutableMessage message;

        try {
            message = new ImmutableMessage(serializedMessage);
        } catch (EncodingException | UnsuppportedVersionException e) {
            throw new JoynrHttpException(Status.BAD_REQUEST, JOYNRMESSAGINGERROR_DESERIALIZATIONFAILED);
        }

        try {
            log.debug("POST message to channel: {} message: {}", ccid, message);

            // TODO Can this happen at all with empty path parameter???
            if (ccid == null) {
                log.error("POST message to channel: NULL. message: {} dropped because: channel Id was not set",
                          message);
                throw new JoynrHttpException(Status.BAD_REQUEST, JOYNRMESSAGINGERROR_CHANNELNOTSET);
            }

            // send the message to the receiver.
            if (message.getTtlMs() == 0) {
                log.error("POST message to channel: {} message: {} dropped because: TTL not set", ccid, message);
                throw new JoynrHttpException(Status.BAD_REQUEST, JOYNRMESSAGINGERROR_EXPIRYDATENOTSET);
            }

            if (message.getTtlMs() < timestampProvider.getCurrentTime()) {
                log.warn("POST message {} to cluster controller: {} dropped because: TTL expired",
                         ccid,
                         message.getId());
                throw new JoynrHttpException(Status.BAD_REQUEST,
                                             JOYNRMESSAGINGERROR_EXPIRYDATEEXPIRED,
                                             request.getRemoteHost());
            }

            if (!messagingService.isAssignedForChannel(ccid)) {
                // scalability extensions: in case that other component should handle this channel
                log.debug("POST message {}: Bounce Proxy not assigned for channel: {}", message, ccid);

                if (messagingService.hasChannelAssignmentMoved(ccid)) {
                    // scalability extension: in case that this component was responsible before but isn't any more
                    log.debug("POST message {}: Bounce Proxy assignment moved for channel: {}", message, ccid);
                    return Response.status(410 /* Gone */).build();
                } else {
                    log.debug("POST message {}: channel  unknown: {}", message, ccid);
                    return Response.status(404 /* Not Found */).build();
                }
            }

            // if the receiver has never registered with the bounceproxy
            // (or his registration has expired) then return 204 no
            // content.
            if (!messagingService.hasMessageReceiver(ccid)) {
                log.debug("POST message {}: no receiver for channel: {}", message, ccid);
                return Response.noContent().build();
            }

            messagingService.passMessageToReceiver(ccid, serializedMessage);

            // the location that can be queried to get the message
            // status
            // TODO REST URL for message status?
            URI location = ui.getBaseUriBuilder().path("messages/" + message.getId()).build();

            // encode URL in case we use sessions
            String encodeURL = response.encodeURL(location.toString());

            // return the message status location to the sender.
            return Response.created(URI.create(encodeURL)).header("msgId", message.getId()).build();

        } catch (WebApplicationException e) {
            throw e;
        } catch (Exception e) {
            log.debug("POST message to channel: {} error: {}", ccid, e.getMessage());
            throw new WebApplicationException(e, Status.INTERNAL_SERVER_ERROR);
        }

    }

}
