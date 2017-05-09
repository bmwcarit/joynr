package io.joynr.messaging;

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

import static io.joynr.messaging.datatypes.JoynrMessagingErrorCode.JOYNRMESSAGINGERROR_CHANNELNOTSET;
import static io.joynr.messaging.datatypes.JoynrMessagingErrorCode.JOYNRMESSAGINGERROR_EXPIRYDATENOTSET;
import static io.joynr.messaging.datatypes.JoynrMessagingErrorCode.JOYNRMESSAGINGERROR_DESERIALIZATIONFAILED;
import io.joynr.communications.exceptions.JoynrHttpException;
import io.joynr.dispatcher.ServletMessageReceiver;
import io.joynr.smrf.EncodingException;
import io.joynr.smrf.UnsuppportedVersionException;

import java.io.IOException;
import java.net.URI;
import java.util.Collection;

import javax.ws.rs.Consumes;
import javax.ws.rs.GET;
import javax.ws.rs.POST;
import javax.ws.rs.Path;
import javax.ws.rs.PathParam;
import javax.ws.rs.Produces;
import javax.ws.rs.WebApplicationException;
import javax.ws.rs.core.Context;
import javax.ws.rs.core.GenericEntity;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;
import javax.ws.rs.core.Response.Status;
import javax.ws.rs.core.UriInfo;

import joynr.ImmutableMessage;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonParseException;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.google.common.base.Charsets;
import com.google.inject.Inject;

@Path("/channels")
/**
 * ChanelService is used by the messaging service of a cluster controller
 * to register for messages from other cluster controllers
 *
 * The following characters are allowed in the id
 * - upper and lower case characters
 * - numbers
 * - underscore (_) hyphen (-) and dot (.)
 */
public class MessagingService {

    private static final Logger log = LoggerFactory.getLogger(MessagingService.class);

    @Context
    UriInfo ui;

    private final IServletMessageReceivers messageReceivers;

    @Inject
    public MessagingService(final IServletMessageReceivers messageReceivers) {
        this.messageReceivers = messageReceivers;
    }

    /**
     * A simple HTML list view of channels. A JSP is used for rendering.
     *
     * @return a HTML list of available channels, and their resources (long poll connections) if connected.
     */
    @GET
    @Produces("application/json")
    public GenericEntity<Collection<ServletMessageReceiver>> listChannels() {
        try {
            return new GenericEntity<Collection<ServletMessageReceiver>>(messageReceivers.getAllServletMessageReceivers()) {
            };
        } catch (Exception e) {
            log.error("GET channels listChannels: error: {}", e.getMessage(), e);
            throw new WebApplicationException(Status.INTERNAL_SERVER_ERROR);
        }

    }

    /**
     * Send a message to the servlet
     * @param ccid the channel id
     * @param messageString the message to be send
     * @return Response
     * @throws IOException in case of IO error occurred
     * @throws JsonParseException in case JSON could not be parsed
     * @throws JsonMappingException in case JSON could not be mapped
     */
    @POST
    @Consumes({ MediaType.TEXT_PLAIN })
    @Path("/{ccid: [A-Z,a-z,0-9,_,\\-,\\.]+}/messageWithoutContentType")
    public Response postMessageWithoutContentType(@PathParam("ccid") String ccid, String messageString)
                                                                                                       throws IOException,
                                                                                                       JsonParseException,
                                                                                                       JsonMappingException {
        ImmutableMessage message;
        try {
            message = new ImmutableMessage(messageString.getBytes(Charsets.UTF_8));
        } catch (EncodingException | UnsuppportedVersionException exception) {
            throw new JoynrHttpException(Status.BAD_REQUEST, JOYNRMESSAGINGERROR_DESERIALIZATIONFAILED);
        }
        return postMessage(ccid, message);
    }

    /**
     * Send a message to the servlet.
     *
     * @param channelId
     *            channel id of the receiver.
     * @param message
     *            the content being sent.
     * @return a location for querying the message status
     */
    @POST
    @Path("/{channelId: [A-Z,a-z,0-9,_,\\-,\\.]+}/message")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response postMessage(@PathParam("channelId") String channelId, ImmutableMessage message) {
        try {
            log.debug("POST message to channel: " + channelId + " message: " + message);

            if (channelId == null) {
                log.error("POST message to channel: NULL. message: {} dropped because: channel Id was not set", message);
                throw new JoynrHttpException(Status.BAD_REQUEST, JOYNRMESSAGINGERROR_CHANNELNOTSET);
            }

            // send the message to the receiver.
            if (message.getTtlMs() == 0) {
                log.error("POST message to channel: {} message: {} dropped because: TTL not set", channelId, message);
                throw new JoynrHttpException(Status.BAD_REQUEST, JOYNRMESSAGINGERROR_EXPIRYDATENOTSET);
            }

            // pass off the message to the registered listener
            ServletMessageReceiver messageReceiver = messageReceivers.getServletMessageReceiver(channelId);

            // messageReceiver is guaranteed to be nonnull for getReceiverForChannelId, so the check is not needed
            // anymore.
            // Update: this is not true, messageReceiver can be null!
            if (messageReceiver == null) {
                log.error("POST message to channel: {} message: {} no receiver for the given channel",
                          channelId,
                          message);
                return Response.noContent().build();
            }

            log.trace("passing off message to messageReceiver: " + channelId);
            messageReceiver.receive(message);

            // the location that can be queried to get the message
            // status
            // TODO REST URL for message status?
            URI location = ui.getBaseUriBuilder().path("messages/" + message.getId()).build();
            // return the message status location to the sender.
            return Response.created(location).build();

        } catch (WebApplicationException e) {
            throw e;
        } catch (Exception e) {
            log.error("POST message to channel: " + channelId + "error: " + e.getMessage(), e);
            throw new WebApplicationException(Status.INTERNAL_SERVER_ERROR);
        }
    }
}
