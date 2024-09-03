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

public class PerformanceTestData {
    private String testName;
    private RequestDurationData requestDurationData;
    private int numberOfCreatedProxies;

    public PerformanceTestData(String testName) {
        this.testName = testName;
        this.requestDurationData = new RequestDurationData();
        this.numberOfCreatedProxies = 0;
    }

    public PerformanceTestData(String testName, RequestDurationData requestDurationData, int numberOfProcessedProxies) {
        this.testName = testName;
        this.requestDurationData = (requestDurationData != null) ? new RequestDurationData(requestDurationData) : null;
        this.numberOfCreatedProxies = numberOfProcessedProxies;
    }

    public RequestDurationData getRequestDurationData() {
        return (requestDurationData != null) ? new RequestDurationData(requestDurationData) : null;
    }

    public void setRequestDurationData(RequestDurationData durationData) {
        this.requestDurationData = (durationData != null) ? new RequestDurationData(durationData) : null;
    }

    public int getNumberOfCreatedProxies() {
        return this.numberOfCreatedProxies;
    }

    public void setNumberOfCreatedProxies(int numberOfProxies) {
        this.numberOfCreatedProxies = numberOfProxies;
    }

    public String getTestName() {
        return this.testName;
    }

    public void setTestName(String testName) {
        this.testName = testName;
    }
}
