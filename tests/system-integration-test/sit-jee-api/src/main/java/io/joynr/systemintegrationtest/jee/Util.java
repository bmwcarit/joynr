/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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
package io.joynr.systemintegrationtest.jee;

import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class Util {

    private static final Logger logger = LoggerFactory.getLogger(Util.class);

    public static void addStacktraceToResultString(Exception theException, StringBuffer result) {
        try (StringWriter writer = new StringWriter(); PrintWriter printWriter = new PrintWriter(writer)) {
            theException.printStackTrace(printWriter);
            result.append(writer.toString());
        } catch (IOException e) {
            String errorMsg = "Unable to add exception stacktrace to result string: " + e;
            result.append(errorMsg);
            logger.error(errorMsg);
        }
    }
}
