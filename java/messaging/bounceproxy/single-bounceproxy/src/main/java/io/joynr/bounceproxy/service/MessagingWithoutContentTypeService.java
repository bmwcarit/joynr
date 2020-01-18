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
package io.joynr.bounceproxy.service;

import java.io.IOException;
import java.net.URI;

import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.ws.rs.Consumes;
import javax.ws.rs.POST;
import javax.ws.rs.Path;
import javax.ws.rs.PathParam;
import javax.ws.rs.WebApplicationException;
import javax.ws.rs.core.Context;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;
import javax.ws.rs.core.UriInfo;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;

import io.joynr.messaging.bounceproxy.LongPollingMessagingDelegate;
import io.joynr.smrf.EncodingException;
import io.joynr.smrf.UnsuppportedVersionException;
import joynr.ImmutableMessage;

@Path("/channels/{ccid: [A-Z,a-z,0-9,_,\\-,\\.]+}/messageWithoutContentType")
/**
 * ChanelService is used by the messaging service of a cluster controller
 * to register for messages from other cluster controllers
 *
 * The following characters are allowed in the id
 * - upper and lower case characters
 * - numbers
 * - underscore (_) hyphen (-) and dot (.)
 */
public class MessagingWithoutContentTypeService {
    private static final Logger log = LoggerFactory.getLogger(MessagingWithoutContentTypeService.class);

    @Context
    UriInfo ui;

    @Context
    HttpServletRequest request;

    @Context
    HttpServletResponse response;

    private final LongPollingMessagingDelegate longPollingDelegate;

    @Inject
    public MessagingWithoutContentTypeService(LongPollingMessagingDelegate longPollingDelegate) {
        this.longPollingDelegate = longPollingDelegate;
    }

    /**
     * Send a message to the given cluster controller like the above method
     * postMessage
     * @param ccid the channel id
     * @param serializedMessage a serialized SMRF message to be sent
     * @return response builder object with the URL that can be queried to get the message
     * @throws IOException on I/O error
     * @throws JsonParseException on parsing problems due to non-well formed content
     * @throws JsonMappingException on fatal problems with mapping of content
     */
    @POST
    @Consumes({ MediaType.APPLICATION_OCTET_STREAM })
    public Response postMessageWithoutContentType(@PathParam("ccid") String ccid,
                                                  byte[] serializedMessage) throws IOException {
        ImmutableMessage message;

        try {
            message = new ImmutableMessage(serializedMessage);
        } catch (EncodingException | UnsuppportedVersionException e) {
            log.error("Failed to deserialize SMRF message: {}", e.getMessage());
            throw new WebApplicationException(e);
        }

        try {
            String msgId = message.getId();
            log.debug("******POST message {} to cluster controller: {}", msgId, ccid);
            log.trace("******POST message {} to cluster controller: {} extended info: \r\n {}", msgId, ccid, message);

            response.setCharacterEncoding("UTF-8");

            // the location that can be queried to get the message
            // status
            // TODO REST URL for message status?
            String path = longPollingDelegate.postMessage(ccid, serializedMessage);

            URI location = ui.getBaseUriBuilder().path(path).build();
            // return the message status location to the sender.
            return Response.created(location).header("msgId", msgId).build();

        } catch (WebApplicationException e) {
            log.error("Invalid request from host {}", request.getRemoteHost());
            throw e;
        } catch (Exception e) {
            log.error("POST message for cluster controller: error: {}", e.getMessage());
            throw new WebApplicationException(e);
        }
    }

}
