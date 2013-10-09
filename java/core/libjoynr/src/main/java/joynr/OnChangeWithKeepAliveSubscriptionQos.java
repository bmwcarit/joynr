package joynr;

/*
 * #%L
 * joynr::java::core::libjoynr
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

import io.joynr.pubsub.HeartbeatSubscriptionInformation;
import io.joynr.pubsub.SubscriptionQos;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.annotation.JsonIgnore;

public class OnChangeWithKeepAliveSubscriptionQos extends OnChangeSubscriptionQos implements
        HeartbeatSubscriptionInformation {
    private static final Logger logger = LoggerFactory.getLogger(OnChangeWithKeepAliveSubscriptionQos.class);

    private static final long MIN_MAX_INTERVAL = 50L;
    private static final long MAX_MAX_INTERVAL = 2592000000L; // 30 days

    private static final long MAX_ALERT_AFTER_INTERVAL = 2592000000L; // 30 days
    private static final long NO_ALERT_AFTER_INTERVAL = 0;
    private static final long DEFAULT_ALERT_AFTER_INTERVAL = NO_ALERT_AFTER_INTERVAL;

    private long maxInterval;
    private long alertAfterInterval;

    protected OnChangeWithKeepAliveSubscriptionQos() {
    }

    /**
     * @param minInterval_ms
     *            defines how often an update may be sent
     * @param maxInterval_ms
     *            defines how long to wait before sending an update even if the value did not change
     * @param expiryDate
     *            how long is the subscription valid
     */
    public OnChangeWithKeepAliveSubscriptionQos(long minInterval_ms, long maxInterval_ms, long expiryDate) {
        this(minInterval_ms,
             maxInterval_ms,
             expiryDate,
             DEFAULT_ALERT_AFTER_INTERVAL,
             SubscriptionQos.DEFAULT_PUBLICATION_TTL);
    }

    /**
     * @param minInterval_ms
     *            defines how often an update may be sent
     * @param maxInterval_ms
     *            defines how long to wait before sending an update even if the value did not change
     * @param expiryDate
     *            how long is the subscription valid
     * @param alertAfterInterval_ms
     *            defines how long to wait for an update before publicationMissed is called
     */
    public OnChangeWithKeepAliveSubscriptionQos(long minInterval_ms,
                                                long maxInterval_ms,
                                                long expiryDate,
                                                long alertAfterInterval_ms) {
        this(minInterval_ms, maxInterval_ms, expiryDate, alertAfterInterval_ms, SubscriptionQos.DEFAULT_PUBLICATION_TTL);
    }

    /**
     * @param minInterval_ms
     *            defines how often an update may be sent
     * @param maxInterval_ms
     *            defines how long to wait before sending an update even if the value did not change
     * @param expiryDate_ms
     *            how long is the subscription valid
     * @param alertAfterInterval_ms
     *            defines how long to wait for an update before publicationMissed is called
     * @param publicationTtl_ms
     *            time to live for publication messages
     */
    public OnChangeWithKeepAliveSubscriptionQos(long minInterval_ms,
                                                long maxInterval_ms,
                                                long expiryDate_ms,
                                                long alertAfterInterval_ms,
                                                long publicationTtl_ms) {
        super(minInterval_ms, expiryDate_ms, publicationTtl_ms);
        setMaxInterval(maxInterval_ms);
        setAlertAfterInterval(alertAfterInterval_ms);
    }

    /**
     * The provider will send notifications every maximum interval in milliseconds, even if the value didn't change. It
     * will send notifications more often if on-change notifications are enabled, the value changes more often, and the
     * minimum interval QoS does not prevent it. The maximum interval can thus be seen as a sort of heart beat.
     * 
     * @return maxInterval_ms The publisher will send a notification at least every maxInterval_ms.
     * 
     */
    public long getMaxInterval() {
        return maxInterval;
    }

    /**
     * The provider will send notifications every maximum interval in milliseconds, even if the value didn't change. It
     * will send notifications more often if on-change notifications are enabled, the value changes more often, and the
     * minimum interval QoS does not prevent it. The maximum interval can thus be seen as a sort of heart beat.
     * 
     * @param maxInterval_ms
     *            The publisher will send a notification at least every maxInterval_ms.
     *            <ul>
     *            <li>The absolute minimum setting is {@value #MIN_MAX_INTERVAL} milliseconds. <br>
     *            Any value less than this minimum will be treated at the absolute minimum setting of
     *            {@value #MIN_MAX_INTERVAL} milliseconds.
     *            <li>The absolute maximum setting is {@value #MAX_MAX_INTERVAL} milliseconds. <br>
     *            Any value bigger than this maximum will be treated as the absolute maximum setting of
     *            {@value #MAX_MAX_INTERVAL} milliseconds.
     *            </ul>
     * 
     */
    public void setMaxInterval(final long maxInterval_ms) {
        if (maxInterval_ms < MIN_MAX_INTERVAL) {
            this.maxInterval = MIN_MAX_INTERVAL;
            return;
        }

        if (maxInterval_ms > MAX_MAX_INTERVAL) {
            this.maxInterval = MAX_MAX_INTERVAL;
            return;
        }

        this.maxInterval = maxInterval_ms;

    }

    /**
     * If no notification was received within the last alert interval, a missed publication notification will be raised.
     * 
     * @return alertInterval_ms If more than alertInterval_ms pass without receiving a message, subscriptionManager will
     *         issue a publicationMissed. If set to -1 never alert.
     */
    public long getAlertAfterInterval() {
        return alertAfterInterval;
    }

    /**
     * Publications will be sent every maximum interval in milliseconds, even if the attribute value has not changed.
     * The maximum interval can be thought of as a keep alive interval, if no other publication has been sent within
     * that time
     * 
     * @param alertAfterInterval_ms
     *            is the max time that can expire without receiving a publication before an alert will be generated. If
     *            more than alertInterval_ms pass without receiving a message, subscriptionManager will issue a
     *            publicationMissed.
     *            <ul>
     *            <li>The value cannot be set below the value of {@link #alertAfterInterval} <br>
     *            Any value less than this minimum will cause no alerts to be generated.
     *            <li>The absolute maximum setting is {@value #MAX_ALERT_AFTER_INTERVAL} milliseconds. <br>
     *            Any value bigger than this maximum will be treated as the absolute maximum setting of
     *            {@value #MAX_ALERT_AFTER_INTERVAL} milliseconds.
     *            </ul>
     */
    public void setAlertAfterInterval(final long alertAfterInterval_ms) {
        if (alertAfterInterval_ms < maxInterval) {
            this.alertAfterInterval = NO_ALERT_AFTER_INTERVAL;
            logger.warn("alertAfterInterval_ms < maxInterval. AlertAfter deactivated");
            return;
        }

        if (alertAfterInterval_ms > MAX_ALERT_AFTER_INTERVAL) {
            this.alertAfterInterval = MAX_ALERT_AFTER_INTERVAL;
            logger.warn("alertAfterInterval_ms > maxInterval. Using MAX_ALERT_AFTER_INTERVAL: {}",
                        MAX_ALERT_AFTER_INTERVAL);
            return;
        }

        this.alertAfterInterval = alertAfterInterval_ms;
    }

    @JsonIgnore
    public long getHeartbeat() {
        return maxInterval;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + (int) (alertAfterInterval ^ (alertAfterInterval >>> 32));
        result = prime * result + (int) (maxInterval ^ (maxInterval >>> 32));
        return result;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj)
            return true;
        if (obj == null)
            return false;
        if (getClass() != obj.getClass())
            return false;
        OnChangeWithKeepAliveSubscriptionQos other = (OnChangeWithKeepAliveSubscriptionQos) obj;
        if (alertAfterInterval != other.alertAfterInterval)
            return false;
        if (maxInterval != other.maxInterval)
            return false;
        return true;
    }

}
