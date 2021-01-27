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

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class PerformanceMeasurementStatistics {
    private static final Logger logger = LoggerFactory.getLogger(PerformanceMeasurementStatistics.class);

    public static void printStatistics(PerformanceTestData testData) {
        logger.info("------- {} was finished! --------", testData.getTestName());
        logger.info("Total duration of test case in ms: {}", testData.getRequestDurationData().getTotalDurationMs());
        logger.info("Number of performed requests: {}",
                    testData.getRequestDurationData().getNumberOfPerformedRequests());
        logger.info("MaxResponseTime: {} [ms]", testData.getRequestDurationData().getMaxRequestResponseTimeMs());
        logger.info("MinResponseTime: {} [ms]", testData.getRequestDurationData().getMinRequestResponseTimeMs());
        logger.info("AverageResponseTime: {} [ms]",
                    testData.getRequestDurationData().getAverageRequestResponseTimeMs());
        logger.info("RequestPerSec: {} [sec]", testData.getRequestDurationData().getNumberOfRequestsPerSecond());
        logger.info("Number of created proxies: {}", testData.getNumberOfCreatedProxies());
        logger.info("-------------------------------------------");
    }
}
