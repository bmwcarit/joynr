/*
 * #%L
 * %%
 * Copyright (C) 2023 BMW Car IT GmbH
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
package io.joynr.test.gcd;

import org.junit.rules.ExternalResource;
import org.junit.runner.Description;
import org.junit.runners.model.Statement;
import org.slf4j.Logger;

public class GcdTestLoggingRule extends ExternalResource {
    private Logger logger;
    private String separator;
    private String beforeString;
    private String afterString;

    public GcdTestLoggingRule(Logger logger) {
        this.logger = logger;
        separator = "[--------]";
        beforeString = "";
        afterString = "";
    }

    @Override
    protected void before() throws Throwable {
        logger.info(separator);
        logger.info(beforeString);
        logger.info(separator);
    }

    @Override
    protected void after() {
        logger.info(separator);
        logger.info(afterString);
        logger.info(separator);
    }

    @Override
    public Statement apply(Statement statement, Description description) {
        String testName = description.getClassName() + "." + description.getMethodName();
        beforeString = "[RUN TEST] " + testName;
        afterString = "[END TEST] " + testName;
        return super.apply(statement, description);
    }

}
