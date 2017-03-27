package io.joynr.bounceproxy.service;

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

import io.joynr.bounceproxy.attachments.AttachmentStorage;

import java.io.UnsupportedEncodingException;

import javax.ws.rs.GET;
import javax.ws.rs.Path;
import javax.ws.rs.Produces;
import javax.ws.rs.QueryParam;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;
import javax.ws.rs.core.Response.Status;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;

@Path("/channels/{ccid: [A-Z,a-z,0-9,_,\\-,\\.]+}/attachment")
public class AttachmentReceiverService {

    private static final Logger log = LoggerFactory.getLogger(AttachmentReceiverService.class);

    private final AttachmentStorage attachmentStorage;

    @Inject
    public AttachmentReceiverService(AttachmentStorage attachmentStorage) {
        this.attachmentStorage = attachmentStorage;
    }

    @GET
    @Produces(MediaType.MULTIPART_FORM_DATA)
    public Response getAttachment(@QueryParam("attachmentId") String attachmentId) {
        // TODO return list of attachments instead of one attachment per message
        byte[] entity = attachmentStorage.get(attachmentId);

        if (entity.length == 0) {
            log.warn("No attachment found for ID {}", attachmentId);
            return Response.status(Status.BAD_REQUEST).build();
        }

        try {
            log.debug("Requested attachment: {}", new String(entity, 0, Math.min(50, entity.length), "UTF-8"));
        } catch (UnsupportedEncodingException e) {
            log.debug("Requested attachment. Failed to log attachment content", e);
        }
        return Response.ok(entity, MediaType.MULTIPART_FORM_DATA).build();
    }
}
