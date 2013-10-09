package io.joynr.demo;

/*
 * #%L
 * joynr::demos::radio-app
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

import java.util.Scanner;
import java.util.regex.Pattern;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class MyRadioHelper {
    private static final Logger LOG = LoggerFactory.getLogger(MyRadioHelper.class);

    @edu.umd.cs.findbugs.annotations.SuppressWarnings(value = "DM_DEFAULT_ENCODING", justification = "Just reading key-input, encoding does not matter here")
    static void pressQEnterToContinue() {
        try {
            // sleep a while to have the log output at the end
            Thread.sleep(1000);
        } catch (InterruptedException e) {
            // nothing to do
        }
        LOG.info("\n\n\n************************************************\n Please press \"q + <Enter>\" to quit application\n************************************************\n\n");
        Scanner input = new Scanner(System.in);
        Pattern pattern = Pattern.compile("q");
        // wait until the user types q to quit
        input.next(pattern);
        input.close();
    }

}
