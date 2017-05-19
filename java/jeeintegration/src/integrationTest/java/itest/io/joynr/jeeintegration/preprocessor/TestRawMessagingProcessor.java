package itest.io.joynr.jeeintegration.preprocessor;


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

import java.io.Serializable;
import java.util.Map;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.base.Charsets;

import io.joynr.messaging.RawMessagingPreprocessor;

public class TestRawMessagingProcessor extends RawMessagingPreprocessor {
    private static final Logger logger = LoggerFactory.getLogger(RawMessagingPreprocessor.class);

    @Override
    public byte[] process(byte[] rawMessage, Map<String, Serializable> context) {
        logger.info("raw message received: " + new String(rawMessage, Charsets.UTF_8));
        return rawMessage;
    }
}
