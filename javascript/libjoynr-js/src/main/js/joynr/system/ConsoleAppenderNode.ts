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
/* istanbul ignore file */

/*eslint no-console: "off"*/
import { LoggingEvent } from "./JoynrLogger";

/**
 * A log4javascript Appender that sends a logged message from a WebWorker to the main context to
 * log it there
 *
 * @name ConsoleAppender
 * @constructor
 */
class ConsoleAppender {
    /**
     * Implementing the appender function of log4javascript appenders
     */
    public append(loggingEvent: LoggingEvent): void {
        const logLevel = loggingEvent.level.name.toLowerCase();
        const formattedMessage = loggingEvent.messages.join(",");
        ((console as any)[logLevel] || console.log)(formattedMessage);
    }

    public toString(): string {
        return "ConsoleAppender";
    }
}

export = ConsoleAppender;
