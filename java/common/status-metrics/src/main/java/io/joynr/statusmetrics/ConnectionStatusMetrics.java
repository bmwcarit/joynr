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

import java.util.Date;
import java.util.Optional;

public class ConnectionStatusMetrics {

    private Object isConnectedLock = new Object();
    private Optional<String> gbid;
    private String url;

    private boolean isSender;
    private boolean isReceiver;

    private boolean isConnected;
    private Date lastStateChange;

    private long receivedMessages;
    private long sentMessages;

    private long connectionDrops;
    private long connectionAttempts;

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

    public void setConnected(boolean isConnected) {
        synchronized (isConnectedLock) {
            this.isConnected = isConnected;
        }
    }

    public void setLastStateChange(Date lastStateChange) {
        this.lastStateChange = Date.from(lastStateChange.toInstant());
    }

    public void increaseReceivedMessages() {
        receivedMessages++;
    }

    public void increaseSentMessages() {
        sentMessages++;
    }

    public void increaseConnectionDrops() {
        connectionDrops++;
    }

    public void increaseConnectionAttempts() {
        connectionAttempts++;
    }

    public Optional<String> getGbid() {
        return gbid;
    }

    public String getUrl() {
        return url;
    }

    public boolean isSender() {
        return isSender;
    }

    public boolean isReceiver() {
        return isReceiver;
    }

    public boolean isConnected() {
        return isConnected;
    }

    public Date getLastStateChange() {
        return Date.from(lastStateChange.toInstant());
    }

    public long getReceivedMessages() {
        return receivedMessages;
    }

    public long getSentMessages() {
        return sentMessages;
    }

    public long getConnectionDrops() {
        return connectionDrops;
    }

    public long getConnectionAttempts() {
        return connectionAttempts;
    }
}
