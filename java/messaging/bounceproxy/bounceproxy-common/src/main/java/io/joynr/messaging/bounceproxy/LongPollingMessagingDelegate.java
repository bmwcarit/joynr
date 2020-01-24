/*
 * #%L
 * joynr::java::messaging::bounceproxy::controlled-bounceproxy
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
package io.joynr.messaging.bounceproxy;

import static io.joynr.messaging.datatypes.JoynrMessagingErrorCode.JOYNRMESSAGINGERROR_CHANNELNOTFOUND;
import static io.joynr.messaging.datatypes.JoynrMessagingErrorCode.JOYNRMESSAGINGERROR_CHANNELNOTSET;
import static io.joynr.messaging.datatypes.JoynrMessagingErrorCode.JOYNRMESSAGINGERROR_DESERIALIZATIONFAILED;
import static io.joynr.messaging.datatypes.JoynrMessagingErrorCode.JOYNRMESSAGINGERROR_EXPIRYDATEEXPIRED;
import static io.joynr.messaging.datatypes.JoynrMessagingErrorCode.JOYNRMESSAGINGERROR_EXPIRYDATENOTSET;
import static io.joynr.messaging.datatypes.JoynrMessagingErrorCode.JOYNRMESSAGINGERROR_RELATIVE_TTL_UNSPORTED;

import java.util.Collection;
import java.util.LinkedList;
import java.util.List;
import java.util.Optional;

import javax.ws.rs.core.Response.Status;

import org.atmosphere.cache.UUIDBroadcasterCache;
import org.atmosphere.cpr.AtmosphereResource;
import org.atmosphere.cpr.Broadcaster;
import org.atmosphere.cpr.BroadcasterFactory;
import org.atmosphere.jersey.Broadcastable;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.communications.exceptions.JoynrHttpException;
import io.joynr.messaging.datatypes.JoynrMessagingErrorCode;
import io.joynr.messaging.info.ChannelInformation;
import io.joynr.smrf.EncodingException;
import io.joynr.smrf.UnsuppportedVersionException;
import joynr.ImmutableMessage;

/**
 * Class that implements long poll messaging routines to be used by bounce
 * proxies.
 *
 * @author christina.strobel
 *
 */
public class LongPollingMessagingDelegate {

    private static final Logger log = LoggerFactory.getLogger(LongPollingMessagingDelegate.class);

    /**
     * Gets a list of all channel information.
     *
     * @return list of all channel informations
     */
    public List<ChannelInformation> listChannels() {
        LinkedList<ChannelInformation> entries = new LinkedList<ChannelInformation>();
        Collection<Broadcaster> broadcasters = BroadcasterFactory.getDefault().lookupAll();
        String name;
        for (Broadcaster broadcaster : broadcasters) {
            if (broadcaster instanceof BounceProxyBroadcaster) {
                name = ((BounceProxyBroadcaster) broadcaster).getName();
            } else {
                name = broadcaster.getClass().getSimpleName();
            }

            Integer cachedSize = null;
            entries.add(new ChannelInformation(name,
                                               broadcaster.getAtmosphereResources().size(),
                                               Optional.ofNullable(cachedSize)));
        }

        return entries;
    }

    /**
     * Creates a long polling channel.
     *
     * @param ccid
     *            the identifier of the channel
     * @param atmosphereTrackingId
     *            the tracking ID of the channel
     * @return the path segment for the channel. The path, appended to the base
     *         URI of the channel service, can be used to post messages to the
     *         channel.
     */
    public String createChannel(String ccid, String atmosphereTrackingId) {

        throwExceptionIfTrackingIdnotSet(atmosphereTrackingId);

        log.info("CREATE channel for cluster controller: {} trackingId: {} ", ccid, atmosphereTrackingId);
        Broadcaster broadcaster = null;
        // look for an existing broadcaster

        BroadcasterFactory defaultBroadcasterFactory = BroadcasterFactory.getDefault();
        if (defaultBroadcasterFactory == null) {
            throw new JoynrHttpException(500, 10009, "broadcaster was null");
        }

        broadcaster = defaultBroadcasterFactory.lookup(Broadcaster.class, ccid, false);
        // create a new one if none already exists
        if (broadcaster == null) {
            broadcaster = defaultBroadcasterFactory.get(BounceProxyBroadcaster.class, ccid);

        }

        // avoids error where previous long poll from browser got message
        // destined for new long poll
        // especially as seen in js, where every second refresh caused a fail
        for (AtmosphereResource resource : broadcaster.getAtmosphereResources()) {
            if (resource.uuid() != null && resource.uuid().equals(atmosphereTrackingId)) {
                resource.resume();
            }
        }

        UUIDBroadcasterCache broadcasterCache = (UUIDBroadcasterCache) broadcaster.getBroadcasterConfig()
                                                                                  .getBroadcasterCache();
        broadcasterCache.activeClients().put(atmosphereTrackingId, System.currentTimeMillis());

        // BroadcasterCacheInspector is not implemented corrected in Atmosphere
        // 1.1.0RC4
        // broadcasterCache.inspector(new MessageExpirationInspector());

        return "/channels/" + ccid + "/";
    }

    /**
     * Deletes a channel from the broadcaster.
     *
     * @param ccid
     *            the channel to delete
     * @return <code>true</code> if the channel existed and could be deleted,
     *         <code>false</code> if there was no channel for the given ID and
     *         therefore could not be deleted.
     */
    public boolean deleteChannel(String ccid) {
        log.info("DELETE channel for cluster controller: " + ccid);
        Broadcaster broadcaster = BroadcasterFactory.getDefault().lookup(Broadcaster.class, ccid, false);
        if (broadcaster == null) {
            return false;
        }

        BroadcasterFactory.getDefault().remove(ccid);

        broadcaster.resumeAll();
        broadcaster.destroy();
        // broadcaster.getBroadcasterConfig().forceDestroy();
        return true;
    }

    /*
     * Opens a channel for long polling.
     *
     * @param ccid
     *            the ID of the channel
     * @param atmosphereTrackingId
     *            the tracking ID
     * @return
     * @throws JoynrHttpException
     *             if no channel with the given ID was found (e.g. because it
     *             wasn't created before) or if the tracking ID wasn't set
     */
    public Broadcastable openChannel(String ccid, String atmosphereTrackingId) {

        throwExceptionIfTrackingIdnotSet(atmosphereTrackingId);

        log.debug("GET Channels open long poll channelId: {} trackingId: {}", ccid, atmosphereTrackingId);
        // NOTE: as of Atmosphere 0.8.5: even though the parameter is set
        // not to create the broadcaster if not
        // found, if the
        // broadcaster is found, but set to "destroyed" then it is recreated
        // TODO when is a broadcaster "destroyed" ???
        Broadcaster broadcaster = BroadcasterFactory.getDefault().lookup(BounceProxyBroadcaster.class, ccid, false);
        if (broadcaster == null) {
            log.error("no broadcaster registered for channel {}", ccid);
            // broadcaster not found for given ccid
            throw new JoynrHttpException(Status.BAD_REQUEST, JOYNRMESSAGINGERROR_CHANNELNOTFOUND);
        }

        // this causes the long poll, or immediate response if elements are
        // in the cache
        return new Broadcastable(broadcaster);
    }

    /**
     * Posts a message to a long polling channel.
     *
     * @param ccid
     *            the identifier of the long polling channel
     * @param serializedMessage
     *            the message to send serialized as a SMRF message
     * @return the path segment for the message status. The path, appended to
     *         the base URI of the messaging service, can be used to query the
     *         message status
     *
     * @throws JoynrHttpException
     *             if one of:
     *             <ul>
     *             <li>ccid is not set</li>
     *             <li>the message has expired or not expiry date is set</li>
     *             <li>no channel registered for ccid</li>
     *             </ul>
     */
    public String postMessage(String ccid, byte[] serializedMessage) {
        ImmutableMessage message;

        try {
            message = new ImmutableMessage(serializedMessage);
        } catch (EncodingException | UnsuppportedVersionException e) {
            throw new JoynrHttpException(Status.BAD_REQUEST, JOYNRMESSAGINGERROR_DESERIALIZATIONFAILED);
        }

        if (ccid == null) {
            log.error("POST message {} to cluster controller: NULL. Dropped because: channel Id was not set.",
                      message.getId());

            throw new JoynrHttpException(Status.BAD_REQUEST, JOYNRMESSAGINGERROR_CHANNELNOTSET);
        }

        // send the message to the receiver.
        if (message.getTtlMs() == 0) {
            log.error("POST message {} to cluster controller: {} dropped because: expiry date not set",
                      ccid,
                      message.getId());
            throw new JoynrHttpException(Status.BAD_REQUEST, JOYNRMESSAGINGERROR_EXPIRYDATENOTSET);
        }

        // Relative TTLs are not supported yet.
        if (!message.isTtlAbsolute()) {
            throw new JoynrHttpException(Status.BAD_REQUEST, JOYNRMESSAGINGERROR_RELATIVE_TTL_UNSPORTED);
        }

        if (message.getTtlMs() < System.currentTimeMillis()) {
            log.warn("POST message {} to cluster controller: {} dropped because: TTL expired", ccid, message.getId());
            throw new JoynrHttpException(Status.BAD_REQUEST, JOYNRMESSAGINGERROR_EXPIRYDATEEXPIRED);
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

        if (ccBroadcaster.getAtmosphereResources().size() == 0) {
            log.debug("no poll currently waiting for channelId: {}", ccid);
        }
        ccBroadcaster.broadcast(message);

        return "messages/" + message.getId();
    }

    private void throwExceptionIfTrackingIdnotSet(String atmosphereTrackingId) {
        if (atmosphereTrackingId == null || atmosphereTrackingId.isEmpty()) {
            log.error("atmosphereTrackingId NOT SET");
            throw new JoynrHttpException(Status.BAD_REQUEST,
                                         JoynrMessagingErrorCode.JOYNRMESSAGINGERROR_TRACKINGIDNOTSET);
        }
    }

}
