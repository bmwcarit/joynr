package io.joynr.bounceproxy.service;

/*
 * #%L
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

import static io.joynr.messaging.datatypes.JoynrMessagingErrorCode.JOYNRMESSAGINGERROR_CHANNELNOTFOUND;
import static io.joynr.messaging.datatypes.JoynrMessagingErrorCode.JOYNRMESSAGINGERROR_CHANNELNOTSET;
import static io.joynr.messaging.datatypes.JoynrMessagingErrorCode.JOYNRMESSAGINGERROR_EXPIRYDATEEXPIRED;
import static io.joynr.messaging.datatypes.JoynrMessagingErrorCode.JOYNRMESSAGINGERROR_EXPIRYDATENOTSET;
import io.joynr.bounceproxy.BounceProxyConstants;
import io.joynr.bounceproxy.attachments.AttachmentStorage;
import io.joynr.bounceproxy.attachments.InMemoryAttachmentStorage;
import io.joynr.communications.exceptions.JoynrHttpException;
import io.joynr.messaging.MessagingModule;
import io.joynr.messaging.bounceproxy.LongPollingMessagingDelegate;
import io.joynr.messaging.info.ChannelInformation;

import java.io.IOException;
import java.io.InputStream;
import java.io.UnsupportedEncodingException;
import java.net.URI;
import java.util.List;
import java.util.concurrent.TimeUnit;

import javax.servlet.ServletContext;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.ws.rs.Consumes;
import javax.ws.rs.DELETE;
import javax.ws.rs.GET;
import javax.ws.rs.HeaderParam;
import javax.ws.rs.POST;
import javax.ws.rs.Path;
import javax.ws.rs.PathParam;
import javax.ws.rs.Produces;
import javax.ws.rs.QueryParam;
import javax.ws.rs.WebApplicationException;
import javax.ws.rs.core.Context;
import javax.ws.rs.core.GenericEntity;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;
import javax.ws.rs.core.Response.Status;
import javax.ws.rs.core.UriInfo;

import joynr.JoynrMessage;

import org.atmosphere.annotation.Suspend;
import org.atmosphere.cpr.Broadcaster;
import org.atmosphere.cpr.BroadcasterFactory;
import org.atmosphere.jersey.Broadcastable;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonParseException;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.sun.jersey.multipart.FormDataParam;
import com.sun.jersey.spi.container.servlet.WebConfig;

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
public class ChannelService {
    private static final Injector injector = Guice.createInjector(new MessagingModule());
    private static final ObjectMapper objectMapper = injector.getInstance(ObjectMapper.class);

    private static final Logger log = LoggerFactory.getLogger(ChannelService.class);
    @Context
    UriInfo ui;

    @Context
    WebConfig wc;

    @Context
    ServletContext servletContext;

    @Context
    HttpServletRequest request;

    @Context
    HttpServletResponse response;
    private static AttachmentStorage attachmentStorage = new InMemoryAttachmentStorage();

    // TODO This is initialized directly until refactoring the bounce proxy is
    // done. Goal is to let bounce proxy implement the xxxRestAdapters and use
    // Guice for injection.
    private final LongPollingMessagingDelegate longPollingDelegate = new LongPollingMessagingDelegate();

    private static final int EXPIRE_TIME_CONNECTION = 20;

    /**
     * A simple HTML list view of channels. A JSP is used for rendering.
     * 
     * @return a HTML list of available channels, and their resources (long poll
     *         connections) if connected.
     */
    @GET
    @Produces("application/json")
    public GenericEntity<List<ChannelInformation>> listChannels() {
        try {
            List<ChannelInformation> entries = longPollingDelegate.listChannels();
            return new GenericEntity<List<ChannelInformation>>(entries) {
            };
        } catch (Throwable e) {
            log.error("GET channels listChannels: error: {}", e.getMessage());
            throw new WebApplicationException(Status.INTERNAL_SERVER_ERROR);
        }

    }

    /**
     * HTTP POST to create a channel, returns location to new resource which can
     * then be long polled. Since the channel id may later change to be a UUID,
     * not using a PUT but rather POST with used id being returned
     * 
     */
    @POST
    @Produces({ MediaType.TEXT_PLAIN })
    public Response createChannel(@QueryParam("ccid") String ccid,
                                  @HeaderParam(BounceProxyConstants.X_ATMOSPHERE_TRACKING_ID) String atmosphereTrackingId) {
        if (ccid == null || ccid.isEmpty())
            throw new JoynrHttpException(Status.BAD_REQUEST, JOYNRMESSAGINGERROR_CHANNELNOTSET);

        try {

            String path = longPollingDelegate.createChannel(ccid, atmosphereTrackingId);
            URI location = ui.getBaseUriBuilder().path(path).build();
            return Response.created(location).entity(ccid).build();

        } catch (WebApplicationException e) {
            throw e;
        } catch (Throwable e) {
            log.error("POST channel for cluster controller: error: " + e.getMessage(), e);
            throw new WebApplicationException(Status.INTERNAL_SERVER_ERROR);
        }
    }

    /**
     * Remove the channel for the given cluster controller.
     * 
     * @param ccid
     * @param broadcaster
     * @return
     */
    @DELETE
    @Path("/{ccid: [A-Z,a-z,0-9,_,\\-,\\.]+}")
    public Response deleteChannel(@PathParam("ccid") String ccid) {
        try {

            if (longPollingDelegate.deleteChannel(ccid)) {
                return Response.ok().build();
            }
            return Response.noContent().build();

        } catch (WebApplicationException e) {
            throw e;
        } catch (Throwable e) {
            log.error("DELETE channel for cluster controller: error: {}" + e.getMessage());
            throw new WebApplicationException(Status.INTERNAL_SERVER_ERROR);
        }
    }

    /**
     * Open a long poll chanel for the given cluster controller The channel is
     * closed automatically by the server at regular intervals to ensure
     * liveliness.
     * 
     * @param ccid
     * @return new message(s), or nothing if the channel is closed by the servr
     */
    @GET
    @Path("/{ccid: [A-Z,a-z,0-9,_,\\-,\\.]+}")
    @Produces({ MediaType.APPLICATION_JSON })
    @Suspend(resumeOnBroadcast = true, period = EXPIRE_TIME_CONNECTION, timeUnit = TimeUnit.SECONDS, contentType = MediaType.APPLICATION_JSON)
    public Broadcastable open(@PathParam("ccid") String ccid,
                              @HeaderParam("X-Cache-Index") Integer cacheIndex,
                              @HeaderParam(BounceProxyConstants.X_ATMOSPHERE_TRACKING_ID) String atmosphereTrackingId) {

        try {

            return longPollingDelegate.openChannel(ccid, atmosphereTrackingId, request.getRemoteHost());
        } catch (WebApplicationException e) {
            throw e;
        } catch (Throwable e) {
            log.error("GET Channels open long poll ccid: error: {}", e.getMessage());
            throw new WebApplicationException(Status.INTERNAL_SERVER_ERROR);
        }

    }

    /**
     * Send a message to the given cluster controller.
     * 
     * @param ccid
     *            channel id of the receiver.
     * @param message
     *            the content being sent.
     * @return a location for querying the message status
     */
    @POST
    @Path("/{ccid: [A-Z,a-z,0-9,_,\\-,\\.]+}/message")
    @Consumes({ MediaType.APPLICATION_JSON })
    public Response postMessage(@PathParam("ccid") String ccid, JoynrMessage joynrMessage) {
        return postMessageInternal(ccid, joynrMessage);
    }

    /**
     * Send a message to the given cluster controller like the above method
     * postMessage
     */
    @POST
    @Consumes({ MediaType.TEXT_PLAIN })
    @Path("/{ccid: [A-Z,a-z,0-9,_,\\-,\\.]+}/messageWithoutContentType")
    public Response postMessageWithoutContentType(@PathParam("ccid") String ccid, String messageString)
                                                                                                       throws IOException,
                                                                                                       JsonParseException,
                                                                                                       JsonMappingException {
        JoynrMessage message = objectMapper.readValue(messageString, JoynrMessage.class);
        return postMessageInternal(ccid, message);
    }

    @POST
    @Path("/{ccid: [A-Z,a-z,0-9,_,\\-,\\.]+}/message")
    @Consumes(MediaType.MULTIPART_FORM_DATA)
    @Produces(MediaType.APPLICATION_JSON)
    public Response postMessageWithAttachment(@PathParam("ccid") String ccid,
                                              @FormDataParam("wrapper") String serializedMessage,
                                              @FormDataParam("attachment") InputStream attachment) {

        try {
            JoynrMessage message = objectMapper.readValue(serializedMessage, JoynrMessage.class);
            log.debug("Message with attachment received! Expiry Date : " + message.getExpiryDate());
            int bytesRead = 0;
            int bytesToRead = 1024;
            byte[] input = new byte[bytesToRead];
            while (bytesRead < bytesToRead) {
                int result = attachment.read(input, bytesRead, bytesToRead - bytesRead);
                if (result == -1)
                    break;
                bytesRead += result;
            }
            attachmentStorage.put(message.getId(), input);

            log.debug("Uploaded attachment : " + new String(input, 0, Math.min(50, input.length), "UTF-8"));
            return postMessageInternal(ccid, message);
        } catch (JsonParseException e) {
            return Response.serverError().build();
        } catch (JsonMappingException e) {
            return Response.serverError().build();
        } catch (IOException e) {
            return Response.serverError().build();
        }
    }

    private Response postMessageInternal(String ccid, JoynrMessage message) {
        try {
            String msgId = message.getId();
            log.debug("******POST message {} to cluster controller: {}", msgId, ccid);
            log.trace("******POST message {} to cluster controller: {} extended info: \r\n {}", ccid, message);

            if (ccid == null) {
                log.error("POST message {} to cluster controller: NULL. Dropped because: channel Id was not set. Request from: {}",
                          message.getId(),
                          request.getRemoteHost());

                throw new JoynrHttpException(Status.BAD_REQUEST, JOYNRMESSAGINGERROR_CHANNELNOTSET);
            }

            // send the message to the receiver.
            if (message.getExpiryDate() == 0) {
                log.error("POST message {} to cluster controller: {} dropped because: expiry date not set",
                          ccid,
                          message.getId());
                throw new JoynrHttpException(Status.BAD_REQUEST,
                                             JOYNRMESSAGINGERROR_EXPIRYDATENOTSET,
                                             request.getRemoteHost());
            }

            if (message.getExpiryDate() < System.currentTimeMillis()) {
                log.warn("POST message {} to cluster controller: {} dropped because: TTL expired",
                         ccid,
                         message.getId());
                throw new JoynrHttpException(Status.BAD_REQUEST,
                                             JOYNRMESSAGINGERROR_EXPIRYDATEEXPIRED,
                                             request.getRemoteHost());
            }

            // look for an existing broadcaster
            Broadcaster ccBroadcaster = BroadcasterFactory.getDefault().lookup(Broadcaster.class, ccid, false);
            if (ccBroadcaster == null) {
                // if the receiver has never registered with the bounceproxy
                // (or his registration has expired) then return 204 no
                // content.
                log.error("POST message {} to cluster controller: {} dropped because: no channel found",
                          ccid,
                          message.getId());
                throw new JoynrHttpException(Status.BAD_REQUEST, JOYNRMESSAGINGERROR_CHANNELNOTFOUND);
            }

            response.setCharacterEncoding("UTF-8");
            if (ccBroadcaster.getAtmosphereResources().size() == 0) {
                log.debug("no poll currently waiting for channelId: {}", ccid);
            }
            ccBroadcaster.broadcast(message);
            // the location that can be queried to get the message
            // status
            // TODO REST URL for message status?
            URI location = ui.getBaseUriBuilder().path("messages/" + msgId).build();
            // return the message status location to the sender.
            return Response.created(location).header("msgId", msgId).build();

        } catch (WebApplicationException e) {
            throw e;
        } catch (Throwable e) {
            log.error("POST message for cluster controller: error: {}", e.getMessage());
            throw new WebApplicationException(e);
        }
    }

    @GET
    @Path("/{ccid: [A-Z,a-z,0-9,_,\\-,\\.]+}/attachment")
    @Produces(MediaType.MULTIPART_FORM_DATA)
    public Response getAttachment(@QueryParam("attachmentId") String attachmentId) {
        // TODO return list of attachments instead of one attachment per message
        byte[] entity = attachmentStorage.get(attachmentId);
        try {
            log.debug("Requested attachment: {}", new String(entity, 0, Math.min(50, entity.length), "UTF-8"));
        } catch (UnsupportedEncodingException e) {
            log.debug("Requested attachment. Failed to log attachment content");
        }
        return Response.ok(entity, MediaType.MULTIPART_FORM_DATA).build();
    }

}
