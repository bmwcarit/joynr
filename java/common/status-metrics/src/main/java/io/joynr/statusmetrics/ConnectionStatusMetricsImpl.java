/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
package io.joynr.statusmetrics;

import java.time.Instant;
import java.util.Optional;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicLong;

public class ConnectionStatusMetricsImpl implements ConnectionStatusMetrics {

    private Optional<String> gbid = Optional.empty();
    private String url;

    private boolean isSender;
    private boolean isReceiver;
    private boolean isReplyReceiver;

    private AtomicBoolean isConnected = new AtomicBoolean();
    private volatile Instant lastStateChange = Instant.now();

    private AtomicLong receivedMessages = new AtomicLong();
    private AtomicLong sentMessages = new AtomicLong();

    private AtomicLong connectionDrops = new AtomicLong();
    private AtomicLong connectionAttempts = new AtomicLong();

    public void setGbid(String gbid) {
        this.gbid = Optional.of(gbid);
    }

    public void setUrl(String url) {
        this.url = url;
    }

    public void setSender(boolean isSender) {
        this.isSender = isSender;
    }

    public void setReceiver(boolean isReceiver) {
        this.isReceiver = isReceiver;
    }

    public void setReplyReceiver(boolean isReplyReceiver) {
        this.isReplyReceiver = isReplyReceiver;
    }

    public void setConnected(boolean isConnected) {
        if (this.isConnected.compareAndSet(!isConnected, isConnected)) {
            lastStateChange = Instant.now();
        }
    }

    public void increaseReceivedMessages() {
        receivedMessages.incrementAndGet();
    }

    public void increaseSentMessages() {
        sentMessages.incrementAndGet();
    }

    public void increaseConnectionDrops() {
        connectionDrops.incrementAndGet();
    }

    public void increaseConnectionAttempts() {
        connectionAttempts.incrementAndGet();
    }

    @Override
    public Optional<String> getGbid() {
        return gbid;
    }

    @Override
    public String getUrl() {
        return url;
    }

    @Override
    public boolean isSender() {
        return isSender;
    }

    @Override
    public boolean isReceiver() {
        return isReceiver;
    }

    @Override
    public boolean isReplyReceiver() {
        return isReplyReceiver;
    }

    @Override
    public boolean isConnected() {
        return isConnected.get();
    }

    @Override
    public Instant getLastConnectionStateChangeDate() {
        return lastStateChange;
    }

    @Override
    public long getReceivedMessages() {
        return receivedMessages.get();
    }

    @Override
    public long getSentMessages() {
        return sentMessages.get();
    }

    @Override
    public long getConnectionDrops() {
        return connectionDrops.get();
    }

    @Override
    public long getConnectionAttempts() {
        return connectionAttempts.get();
    }
}
