package io.joynr.util;

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

import io.joynr.messaging.util.Utilities;

import java.util.List;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

/**
 * Test for the Utilities.splitJson() function.
 * A string of three simplified JSON Objects is split into three individual strings.
 */

public class JsonSplitTest {
    private String originalString;

    @Before
    public void setUp() throws Exception {
        originalString = "{test{inner}} {123}{last}";
    }

    @Test
    public void splittingTest() {
        List<String> splitResults = Utilities.splitJson(originalString);

        Assert.assertEquals("Splitting of first part of Json object failed", "{test{inner}}", splitResults.get(0));
        Assert.assertEquals("Splitting of second part of Json object failed", "{123}", splitResults.get(1));
        Assert.assertEquals("Splitting of third part of Json object failed", "{last}", splitResults.get(2));

        String inputStream;

        inputStream = "{test}";
        splitResults = Utilities.splitJson(inputStream);
        Assert.assertEquals("Splitting failed", "{test}", splitResults.get(0));

        inputStream = "{\"id\":34}";
        splitResults = Utilities.splitJson(inputStream);
        Assert.assertEquals("Splitting failed", "{\"id\":34}", splitResults.get(0));

        //  { within a string should be ok
        inputStream = "{\"messa{ge\":{one:two}}{\"id\":35}";
        splitResults = Utilities.splitJson(inputStream);
        Assert.assertEquals("Splitting failed", 2, splitResults.size());
        Assert.assertEquals("Splitting failed", "{\"messa{ge\":{one:two}}", splitResults.get(0));

        //payload may not contain { or } outside a string.
        inputStream = "{\"id\":3{4}";
        splitResults = Utilities.splitJson(inputStream);
        Assert.assertEquals("Splitting should fail", 0, splitResults.size());

        //  } within a string should be ok
        inputStream = "{\"messa}ge\":{one:two}}{\"id\":35}";
        splitResults = Utilities.splitJson(inputStream);
        Assert.assertEquals("Splitting failed", 2, splitResults.size());
        Assert.assertEquals("Splitting failed", "{\"messa}ge\":{one:two}}", splitResults.get(0));

        //  }{ within a string should be ok
        inputStream = "{\"messa}{ge\":{one:two}}{\"id\":35}";
        splitResults = Utilities.splitJson(inputStream);
        Assert.assertEquals("Splitting failed", 2, splitResults.size());
        Assert.assertEquals("Splitting failed", "{\"messa}{ge\":{one:two}}", splitResults.get(0));

        //  {} within a string should be ok
        inputStream = "{\"messa{}ge\":{one:two}}{\"id\":35}";
        splitResults = Utilities.splitJson(inputStream);
        Assert.assertEquals("Splitting failed", 2, splitResults.size());
        Assert.assertEquals("Splitting failed", "{\"messa{}ge\":{one:two}}", splitResults.get(0));

        //string may contain \"
        inputStream = "{\"mes\\\"sa{ge\":{one:two}}{\"id\":35}";
        //inputStream:{"mes\"sa{ge":{one:two}}{"id":35}
        splitResults = Utilities.splitJson(inputStream);
        Assert.assertEquals("Splitting failed with escaped \" within string", 2, splitResults.size());
        Assert.assertEquals("Splitting failed with escaped \" within string",
                            "{\"mes\\\"sa{ge\":{one:two}}",
                            splitResults.get(0));

        inputStream = "{\"mes\\\\\"sa{ge\":{one:two}}{\"id\":35}";
        // inputStream: {"mes\\"sa{ge":{one:two}}{"id":35}
        // / does not escape within JSON String, so the string should not be ended after mes\"
        splitResults = Utilities.splitJson(inputStream);
        Assert.assertEquals("Splitting failed", 2, splitResults.size());
        Assert.assertEquals("Splitting failed", "{\"mes\\\\\"sa{ge\":{one:two}}", splitResults.get(0));

    }
}
