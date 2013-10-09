package io.joynr.messaging.util;

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

import java.util.List;

import com.google.common.collect.Lists;

public class Utilities {

    /**
     * return an array of the params, useful for logging in slf4j
     *
     * @param args
     * @return
     */
    public static Object[] toArray(Object... args) {
        return args;
    }

    /**
     * This method splits the input string into several json objects.
     * This is needed because Atmosphere sends more than one json in a single response.
     * <p/>
     * For example for the input {{test}{test2}}{test3} it would produce the following list:
     * [{{test}{test2}}, {test3}]
     */
    public static List<String> splitJson(String combinedJsonString) {
        List<String> result = Lists.newArrayList();

        int numberOfOpeningBraces = 0;
        boolean isInsideString = false;
        /*A string starts with an unescaped " and ends with an unescaped "
         *} or { within a string must be ignored.
         */
        StringBuilder jsonBuffer = new StringBuilder();
        for (int i = 0; i < combinedJsonString.length(); i++) {
            char c = combinedJsonString.charAt(i);
            if (c == '"' && i > 0 && combinedJsonString.charAt(i - 1) != '\\') {
                //only switch insideString if " is not escaped
                isInsideString = !isInsideString;
            }
            if (c == '{' && !isInsideString) {
                numberOfOpeningBraces++;
            } else if (c == '}' && !isInsideString) {
                numberOfOpeningBraces--;
            }

            jsonBuffer.append(c);

            if (numberOfOpeningBraces == 0 && jsonBuffer.length() != 0) {
                //Prevent empty strings to be added to the result list in case there are spaces between JSON objects
                if (jsonBuffer.toString().charAt(0) == '{') {
                    result.add(jsonBuffer.toString());
                }
                jsonBuffer.setLength(0);
            }
        }

        return result;
    }

}
