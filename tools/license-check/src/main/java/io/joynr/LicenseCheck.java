package io.joynr;

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
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.HashSet;
import java.util.Set;

public class LicenseCheck {
    private static final String JOYNR_SRC_DIR = "/home/joynr/ws/joynr/dev/cpp/";
    public static final String JOYNR_BUILD_DIR = "/home/joynr/ws/joynr/dev/build/";
    public static final String JOYNR_INCLUDE_DIR = "bin/include/";
    public static final String DEPEND_FILES_FILENAME = "/home/joynr/ws/joynr/dev/build/list-of-depend-files.txt";
    public static final String HEADER_FILES_FILENAME = "/home/joynr/ws/joynr/dev/build/header-files.txt";
    public static final String OBJECT_FILE_DELIMITER = ".o: ";

    private LicenseCheck() {
    }

    public static void main(String[] args) throws IOException {
        Set<String> uniqueHeaderFileNames = new HashSet<>();
        try (BufferedReader dependFiles = new BufferedReader(new FileReader(DEPEND_FILES_FILENAME))) {
            String dependFileName;
            while ((dependFileName = dependFiles.readLine()) != null) {
                try (BufferedReader dependFile = new BufferedReader(new FileReader(dependFileName))) {
                    String line;
                    while ((line = dependFile.readLine()) != null) {
                        if (line.startsWith("#") || line.isEmpty()) {
                            continue;
                        }
                        String headerFileName = line.substring(line.indexOf(OBJECT_FILE_DELIMITER)
                                + OBJECT_FILE_DELIMITER.length());

                        if (headerFileName.matches(".*/moc_.*\\.cxx")) {
                            // Qt moc files
                            continue;
                        }

                        if (headerFileName.startsWith("/home/joynr/QtSDK/Desktop/Qt/4.8.1/gcc/include/QtCore/")
                                || headerFileName.startsWith("/home/joynr/QtSDK/Desktop/Qt/4.8.1/gcc/include/Qt3Support/")) {
                            // QtCore is LGPL v2.1
                            continue;
                        }
                        if (headerFileName.contains("ThirdParty/src/googlemock/")) {
                            // Google Mock is BSD 3-Clause License
                            continue;
                        }
                        if (headerFileName.contains("ThirdParty/src/googletest/")) {
                            // Google Test is BSD 3-Clause License
                            continue;
                        }

                        // make paths absolut
                        if (headerFileName.startsWith(JOYNR_INCLUDE_DIR)) {
                            headerFileName = JOYNR_BUILD_DIR + headerFileName;
                        }
                        if (headerFileName.charAt(0) != '/') {
                            headerFileName = JOYNR_SRC_DIR + headerFileName;
                        }

                        File file = new File(headerFileName);
                        if (!file.exists()) {
                            throw new IllegalArgumentException("File not found: " + headerFileName);
                        }

                        headerFileName = file.getCanonicalPath();

                        if ((headerFileName.startsWith(JOYNR_SRC_DIR) && !headerFileName.contains("libs/"))
                                || (headerFileName.startsWith(JOYNR_BUILD_DIR) && !headerFileName.contains("libs/"))) {
                            // our own stuff
                            continue;
                        }

                        uniqueHeaderFileNames.add(headerFileName);
                    }
                }
            }
        }

        BufferedWriter headerFiles = new BufferedWriter(new FileWriter(HEADER_FILES_FILENAME));
        for (String uniqueHeaderFileName : uniqueHeaderFileNames) {
            headerFiles.write(uniqueHeaderFileName + "\n");
        }
        headerFiles.close();
    }
}
