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
import { Logging } from "../start/interface/Provisioning";
import ConsoleAppender from "./ConsoleAppenderNode";

import JoynrLogger from "./JoynrLogger";
import * as UtilInternal from "../util/UtilInternal";

const logLevelChangedCallbacks: Function[] = [];
class LoggingManager {
    public readonly LogLevel = JoynrLogger.LogLevel;

    public reset(): void {
        JoynrLogger.setOutput(ConsoleAppender.prototype.append);
    }

    /**
     * @param name - the name of the logger
     *
     * @returns a new Instance of JoynrLogger with the respective name.
     */
    public getLogger(name: string): JoynrLogger {
        return new JoynrLogger(name);
    }

    /**
     * configure takes a configuration object that conforms to a subset of the log4j2
     * json configuration
     *
     * @param settings - log4j2-style JSON config, but as JavaScript object
     *            (i.e. already parsed)
     */
    public configure(settings: Logging): void {
        const settingsProxy = UtilInternal.augmentConfig(settings);

        const level = settingsProxy.configuration.loggers.root.level();
        if (level) {
            JoynrLogger.setLogLevel(level);
            logLevelChangedCallbacks.forEach(
                (callback): void => {
                    callback(level);
                }
            );
        }
        const patternLayout = settingsProxy.configuration.appenders.appender[0].PatternLayout.pattern();
        if (patternLayout && patternLayout !== "%m") {
            JoynrLogger.setFormatting(patternLayout);
        }

        if (settings.appenderClasses && Object.keys(settings.appenderClasses).length > 0) {
            const appenderClassKey = Object.keys(settings.appenderClasses)[0];
            JoynrLogger.setOutput(settings.appenderClasses[appenderClassKey].prototype.append);
        }
    }

    public registerForLogLevelChanged(callback: Function): void {
        logLevelChangedCallbacks.push(callback);
    }
}
type loggingManager = LoggingManager;
const loggingManager = new LoggingManager();
loggingManager.reset();

export = loggingManager;
