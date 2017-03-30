package io.joynr.runtime;

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
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.text.MessageFormat;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class RegexTestHarness {

    public static void main(String[] args) {
        Pattern pattern = null;
        while (true) {
            try {
                BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(System.in,
                                                                                         Charset.defaultCharset()));
                System.out.println("%nEnter your regex: ");
                String newPattern = bufferedReader.readLine();
                if (!newPattern.equals("")) {
                    pattern = Pattern.compile(newPattern);
                } else {
                    System.out.println("using previous pattern: " + pattern);
                }
                System.out.println("Enter input string to search: ");
                String searchThisString = bufferedReader.readLine();
                Matcher matcher = pattern.matcher(searchThisString);

                if (matcher.matches()) {

                    System.out.println(MessageFormat.format("I found the text {0} starting at index {1} and ending at index {2}",
                                                            matcher.group(),
                                                            matcher.start(),
                                                            matcher.end()));
                } else {
                    System.err.println("No match found.");
                }
            } catch (Exception e) {
                System.err.println(e);
            }
        }
    }
}