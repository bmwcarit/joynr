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
package io.joynr.jeeintegration.messaging;

import static io.joynr.messaging.datatypes.JoynrMessagingErrorCode.JOYNRMESSAGINGERROR_CHANNELNOTSET;
import static io.joynr.messaging.datatypes.JoynrMessagingErrorCode.JOYNRMESSAGINGERROR_DESERIALIZATIONFAILED;
import static io.joynr.messaging.datatypes.JoynrMessagingErrorCode.JOYNRMESSAGINGERROR_EXPIRYDATENOTSET;
import static java.lang.String.format;

import java.io.IOException;
import java.net.URI;
import java.nio.charset.StandardCharsets;

import javax.inject.Inject;
import javax.ws.rs.Consumes;
import javax.ws.rs.GET;
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

import com.google.inject.Injector;

import io.joynr.communications.exceptions.JoynrHttpException;
import io.joynr.dispatcher.ServletMessageReceiver;
import io.joynr.jeeintegration.JoynrIntegrationBean;
import io.joynr.smrf.EncodingException;
import io.joynr.smrf.UnsuppportedVersionException;
import joynr.ImmutableMessage;

/**
 * The <code>JeeMessagingEndpoint</code> is a JAX-RS endpoint which receives joynr messages to be processed.
 * See {@link #postMessage(String, byte[], UriInfo)} for details.
 * <p>
 * The following characters are allowed in the id - upper and lower case characters - numbers - underscore (_) hyphen
 * (-) and dot (.)
 */
@Path("/channels")
public class JeeMessagingEndpoint {

    private static final Logger logger = LoggerFactory.getLogger(JeeMessagingEndpoint.class);

    private Injector injector;

    private ServletMessageReceiver messageReceiver;

    @Inject
    public JeeMessagingEndpoint(JoynrIntegrationBean jeeIntegrationBean) {
        injector = jeeIntegrationBean.getJoynrInjector();
        messageReceiver = injector.getInstance(ServletMessageReceiver.class);
    }

    @GET
    public Response status() {
        logger.info("Status called.");
        return Response.status(200).build();
    }

    /**
     * Receives a message for the given channel ID, parses the binary content as SMRF, and forwards to
     * {@link #postMessage(String, byte[], UriInfo)}.
     *
     * @param ccid
     *            the channel id.
     * @param serializedMessage
     *            a serialized SMRF message to be send.
     * @param uriInfo
     *            the URI Information for the request being processed.
     * @return Response the endpoint where the status of the message can be queried.
     * @throws IOException
     *             in case of IO error occurred.
     */
    @POST
    @Consumes({ MediaType.APPLICATION_OCTET_STREAM })
    @Path("/{ccid: [A-Z,a-z,0-9,_,\\-,\\.]+}/messageWithoutContentType")
    public Response postMessageWithoutContentType(@PathParam("ccid") String ccid,
                                                  byte[] serializedMessage,
                                                  @Context UriInfo uriInfo) throws IOException {
        return postMessage(ccid, serializedMessage, uriInfo);
    }

    /**
     * Receives a binary encoded {@link ImmutableMessage} which it then routes to the {@link #messageReceiver message
     * receiver} in the joynr runtime.
     *
     * @param channelId
     *            channel id of the receiver.
     * @param serializedMessage
     *            a serialized SMRF message being sent.
     * @param uriInfo
     *            the URI Information for the request being processed.
     * @return a location for querying the message status.
     */
    @POST
    @Path("/{channelId: [A-Z,a-z,0-9,_,\\-,\\.]+}/message")
    @Produces({ MediaType.APPLICATION_OCTET_STREAM })
    public Response postMessage(@PathParam("channelId") String channelId,
                                byte[] serializedMessage,
                                @Context UriInfo uriInfo) {
        if (logger.isDebugEnabled()) {
            logger.debug("Incoming message:\n" + new String(serializedMessage, StandardCharsets.UTF_8));
        }
        try {
            ImmutableMessage message;

            try {
                message = new ImmutableMessage(serializedMessage);
            } catch (EncodingException | UnsuppportedVersionException exception) {
                logger.error("Failed to deserialize message for channelId {}: {}", channelId, exception.getMessage());
                throw new JoynrHttpException(Status.BAD_REQUEST, JOYNRMESSAGINGERROR_DESERIALIZATIONFAILED);
            }

            if (logger.isDebugEnabled()) {
                logger.debug("POST to channel: {} message: {}", channelId, message);
            }

            if (channelId == null) {
                logger.error("POST message to channel: NULL. message: {} dropped because: channel Id was not set",
                             message);
                throw new JoynrHttpException(Status.BAD_REQUEST, JOYNRMESSAGINGERROR_CHANNELNOTSET);
            }

            if (message.getTtlMs() == 0) {
                logger.error("POST message to channel: {} message: {} dropped because: TTL not set",
                             channelId,
                             message);
                throw new JoynrHttpException(Status.BAD_REQUEST, JOYNRMESSAGINGERROR_EXPIRYDATENOTSET);
            }

            if (messageReceiver == null) {
                logger.error("POST message to channel: {} message: {} no receiver for the given channel",
                             channelId,
                             message);
                return Response.noContent().build();
            }

            if (logger.isTraceEnabled()) {
                logger.trace("passing off message to messageReceiver: {}", channelId);
            }
            messageReceiver.receive(message);

            URI location = uriInfo.getBaseUriBuilder().path("messages/" + message.getId()).build();
            return Response.created(location).build();
        } catch (WebApplicationException e) {
            throw e;
        } catch (Exception e) {
            logger.error(format("POST message to channel: %s error: %s", channelId, e.getMessage()), e);
            throw new WebApplicationException(Status.INTERNAL_SERVER_ERROR);
        }
    }

}
