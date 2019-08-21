/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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

import { LoggingEvent } from "joynr/joynr/system/JoynrLogger";

type LogLevel = "debug" | "info" | "log" | "error";

/**
 * An exemplary log4javascript Appender type that can be used via the joynr config. It simply logs
 * the incoming log events to the console.
 *
 * @name CustomerLoggingAppender
 * @constructor
 */
class CustomerLoggingAppender {
    /**
     * Implementing the appender function of log4javascript appenders
     */
    public append(loggingEvent: LoggingEvent): void {
        //console.debug does not exist in nodejs

        const formattedMessage = loggingEvent.messages.join(",");
        const logLevel: LogLevel = loggingEvent.level.name.toLowerCase() as any;
        console[logLevel](formattedMessage);
    }
}

console.debug = console.log;

export = CustomerLoggingAppender;
