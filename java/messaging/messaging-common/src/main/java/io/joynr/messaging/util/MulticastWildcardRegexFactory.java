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
package io.joynr.messaging.util;

import java.util.regex.Pattern;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.exceptions.JoynrIllegalStateException;

public class MulticastWildcardRegexFactory {

    private static final Logger logger = LoggerFactory.getLogger(MulticastWildcardRegexFactory.class);

    public Pattern createIdPattern(String multicastId) {
        verifyMulticastIdValid(multicastId);
        if (multicastId.matches("^\\*$")) {
            logger.trace("Creating multicast ID regex pattern: {}", ".+");
            return Pattern.compile(".+");
        }
        String patternString = multicastId.replaceAll("^\\+(/)?", "[^\\/]+$1");
        patternString = patternString.replaceAll("/\\+/", "/[^\\/]+/");
        if (patternString.endsWith("/+")) {
            patternString = patternString.substring(0, patternString.length() - 1);
            patternString = patternString + "[^/]+$";
        } else if (patternString.endsWith("/*")) {
            patternString = patternString.substring(0, patternString.length() - 2);
            patternString = patternString + "(/.*)?$";
        }
        logger.trace("Creating multicast ID regex pattern: {}", patternString);
        return Pattern.compile(patternString);
    }

    private void verifyMulticastIdValid(String multicastId) {
        boolean invalid = Pattern.compile("[^/]\\+").matcher(multicastId).find()
                || Pattern.compile("\\+[^/]").matcher(multicastId).find()
                || (!"*".equals(multicastId) && multicastId.contains("*") && !multicastId.matches(".*/\\*$"));
        if (invalid) {
            throw new JoynrIllegalStateException("Multicast IDs may only contain '+' as a placeholder for a partition, and '*' as only character or right at the end after a '/'. You passed in: "
                    + multicastId);
        }
    }

}
