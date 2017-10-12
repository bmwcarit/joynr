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
package io.joynr.integration.matchers;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;

import org.hamcrest.BaseMatcher;
import org.hamcrest.Description;
import org.hamcrest.Matcher;

public class InputStreamMatchers {

    public static Matcher<InputStream> equalsLineByLine(final String filename) {
        return new BaseMatcher<InputStream>() {

            private int mismatchLineNo = 0;
            private String mismatchActual = null;
            private String mismatchExpected = null;

            @Override
            public boolean matches(Object item) {

                BufferedReader actualBr = null;
                BufferedReader expectedBr = null;

                try {
                    InputStream actualIn = (InputStream) item;
                    actualBr = new BufferedReader(new InputStreamReader(actualIn));

                    InputStream expectedIn = getClass().getResourceAsStream("/" + filename);
                    expectedBr = new BufferedReader(new InputStreamReader(expectedIn));

                    mismatchLineNo = 0;

                    while ((mismatchActual = actualBr.readLine()) != null) {
                        mismatchLineNo++;
                        mismatchExpected = expectedBr.readLine();

                        if (mismatchExpected == null) {
                            return false;
                        }

                        if (!mismatchActual.trim().equals(mismatchExpected.trim())) {
                            return false;
                        }
                    }

                    if (expectedBr.readLine() != null) {
                        return false;
                    }

                    return true;
                } catch (IOException e) {
                    throw new RuntimeException(e);
                } finally {
                    try {
                        if (actualBr != null)
                            actualBr.close();

                        if (expectedBr != null)
                            expectedBr.close();
                    } catch (IOException e) {
                        throw new RuntimeException(e);
                    }
                }
            }

            @Override
            public void describeMismatch(Object item, Description description) {
                description.appendText("line " + mismatchLineNo + ": " + mismatchActual);
            }

            @Override
            public void describeTo(Description description) {
                description.appendText("line " + mismatchLineNo + ": " + mismatchExpected);
            }

        };
    }
}
