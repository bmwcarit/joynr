/*
 * #%L
 * %%
 * Copyright (C) 2021 BMW Car IT GmbH
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

package io.joynr.performancemeasurement;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.ConcurrentLinkedQueue;

public class RequestDurationData {
    private List<Long> requestDurationList;
    private long totalIterationDurationMs;

    public RequestDurationData() {
        this.requestDurationList = new ArrayList<Long>();
        this.totalIterationDurationMs = 0;
    }

    public RequestDurationData(ConcurrentLinkedQueue<Long> durationQueue, long totalDurationMs) {
        Long[] durationArray = durationQueue.toArray(new Long[0]);
        this.requestDurationList = Arrays.asList(durationArray);
        this.totalIterationDurationMs = totalDurationMs;
    }

    public RequestDurationData(RequestDurationData other) {
        assert other != null : "RequestDurationData must not be null";
        this.requestDurationList = new ArrayList<>(other.requestDurationList);
        this.totalIterationDurationMs = other.totalIterationDurationMs;
    }

    public void setRequestDurationList(List<Long> durationList) {
        this.requestDurationList = (durationList != null) ? new ArrayList<>(this.requestDurationList)
                : new ArrayList<>();
    }

    public List<Long> getRequestDurationList() {
        return new ArrayList<>(this.requestDurationList);
    }

    public void setTotalDurationMs(long totalDurationMs) {
        this.totalIterationDurationMs = totalDurationMs;
    }

    public long getTotalDurationMs() {
        return this.totalIterationDurationMs;
    }

    public long getMaxRequestResponseTimeMs() {
        return this.requestDurationList.isEmpty() ? 0l : Collections.max(this.requestDurationList);
    }

    public long getMinRequestResponseTimeMs() {
        return this.requestDurationList.isEmpty() ? 0l : Collections.min(this.requestDurationList);
    }

    public long getAverageRequestResponseTimeMs() {
        return this.requestDurationList.isEmpty() ? 0l
                : this.requestDurationList.stream().mapToLong(Long::longValue).sum() / this.requestDurationList.size();
    }

    public double getNumberOfRequestsPerSecond() {
        double totalDurationSec = (double) this.totalIterationDurationMs / 1000.0;
        double numberOfRequestsPerSec = (double) this.requestDurationList.size() / totalDurationSec;
        return numberOfRequestsPerSec;
    }

    public int getNumberOfPerformedRequests() {
        return this.requestDurationList.size();
    }
}
