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

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.nio.charset.StandardCharsets;
import java.sql.Timestamp;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class PerformanceMeasurementStatistics {
    private static final Logger logger = LoggerFactory.getLogger(PerformanceMeasurementStatistics.class);

    public static void writeTestDataToCsvFile(PerformanceTestData testData, String fileName) {
        String dataStr = String.format("%s,%s,%d,%d,%f,%d,%d",
                                       new Timestamp(System.currentTimeMillis()),
                                       testData.getTestName(),
                                       testData.getRequestDurationData().getNumberOfPerformedRequests(),
                                       testData.getNumberOfCreatedProxies(),
                                       testData.getRequestDurationData().getNumberOfRequestsPerSecond(),
                                       testData.getRequestDurationData().getAverageRequestResponseTimeMs(),
                                       testData.getRequestDurationData().getTotalDurationMs());
        printStatistics(testData);
        try {
            writeToFile(dataStr, fileName);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

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

    private static void writeToFile(String data, String fileName) throws IOException {
        // create the file and write headers only once
        createFileIfRequired(fileName);

        // write data
        final boolean append = true;
        BufferedWriter dataWriter = new BufferedWriter(new OutputStreamWriter(new FileOutputStream(fileName, append),
                                                                              StandardCharsets.UTF_8));
        dataWriter.write(data);
        dataWriter.newLine(); //Add new line afterwards
        dataWriter.close();
    }

    private static void createFileIfRequired(String csvFile) throws IOException {
        try {
            File file = new File(csvFile);
            if (file.createNewFile()) {
                logger.debug("File created: {}", file.getName());
                final boolean append = true;
                BufferedWriter headerWriter = new BufferedWriter(new OutputStreamWriter(new FileOutputStream(csvFile,
                                                                                                             append),
                                                                                        StandardCharsets.UTF_8));
                final String headerToAppend = "Timestamp,TestName,NumOfRequests,NumOfProxies,NumberOfReqsPerSec,AverageReqResponse[ms],TotalDuration[ms]";
                headerWriter.write(headerToAppend);
                headerWriter.newLine(); //Add new line afterwards
                headerWriter.close();
            } else {
                logger.debug("File already exists. Writing data to: {}", csvFile);
            }
        } catch (IOException e) {
            logger.error("An error occurred while creating csv file.");
            e.printStackTrace();
            throw new IOException("An error occurred while creating csv file: can not continue");
        }
    }
}
