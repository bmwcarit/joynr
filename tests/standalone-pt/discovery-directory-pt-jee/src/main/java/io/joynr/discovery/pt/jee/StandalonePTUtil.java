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
package io.joynr.discovery.pt.jee;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.nio.charset.StandardCharsets;
import java.sql.Timestamp;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class StandalonePTUtil {

    private static final Logger logger = LoggerFactory.getLogger(StandalonePTUtil.class);

    public static synchronized void writeDataToCsvFile(String csvFile, long receivedMessages, long sentMessages) {
        String dataStr = String.format("backend,%s,%d,%d",
                                       new Timestamp(System.currentTimeMillis()),
                                       receivedMessages,
                                       sentMessages);
        try {
            writeToFile(csvFile, dataStr);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private static void writeToFile(String csvFile, String data) throws IOException {
        // create the file and write headers only once
        createFileIfRequired(csvFile);

        // write data
        final boolean append = true;
        BufferedWriter dataWriter = new BufferedWriter(new OutputStreamWriter(new FileOutputStream(csvFile, append),
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
                final String headerToAppend = "ContainerId,Timestamp[uts],ReceivedMessages[msgs/sec],SentMessages[msgs/sec]";
                headerWriter.write(headerToAppend);
                headerWriter.newLine(); //Add new line afterwards
                headerWriter.close();
            } else {
                logger.debug("File already exists. Writting data to: {}", csvFile);
            }
        } catch (IOException e) {
            logger.error("An error occurred while creating csv file.");
            e.printStackTrace();
            throw new IOException("An error occurred while creating csv file: can not continue");
        }
    }
}
