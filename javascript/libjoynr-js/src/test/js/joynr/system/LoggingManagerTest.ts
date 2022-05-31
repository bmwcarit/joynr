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

import LoggingManager from "../../../../main/js/joynr/system/LoggingManager";
import JoynrLogger from "../../../../main/js/joynr/system/JoynrLogger";

describe("libjoynr-js.joynr.util.LoggingManager", () => {
    beforeEach(() => {
        jest.clearAllMocks();
        jest.spyOn(JoynrLogger, "setLogLevel");
        jest.spyOn(JoynrLogger, "setFormatting");
        jest.spyOn(JoynrLogger, "setOutput");
    });

    function expectNoCalls() {
        expect(JoynrLogger.setLogLevel).not.toHaveBeenCalled();
        expect(JoynrLogger.setFormatting).not.toHaveBeenCalled();
        expect(JoynrLogger.setOutput).not.toHaveBeenCalled();
    }

    it("will do nothing when the configuration is missing", () => {
        LoggingManager.configure({});
        expectNoCalls();

        LoggingManager.configure({ configuration: {} } as any);
        expectNoCalls();

        LoggingManager.configure({ configuration: { loggers: {} } } as any);
        expectNoCalls();

        LoggingManager.configure({ configuration: { loggers: { root: {} } } } as any);
        expectNoCalls();

        LoggingManager.configure({ configuration: { appenders: {} } } as any);
        expectNoCalls();

        LoggingManager.configure({
            configuration: { appenders: { appender: [] } }
        });
        expectNoCalls();

        LoggingManager.configure({
            configuration: { appenders: { appender: [{}] } }
        } as any);
        expectNoCalls();

        LoggingManager.configure({
            configuration: { appenders: { appender: [{ PatternLayout: {} }] } }
        } as any);
        expectNoCalls();

        LoggingManager.configure({ appenderClasses: {} });
        expectNoCalls();
    });

    it("sets the loglevel of JoynrLogger when loglevel of the root logger is set", () => {
        LoggingManager.configure({
            configuration: {
                loggers: { root: { level: JoynrLogger.LogLevel.DEBUG } }
            }
        });
        expect(JoynrLogger.setLogLevel).toHaveBeenCalledWith(JoynrLogger.LogLevel.DEBUG);
    });

    it("sets the formatting of JoynrLogger when a patternLayout is set", () => {
        const pattern = "some pattern %m";
        LoggingManager.configure({
            configuration: {
                appenders: { appender: [{ PatternLayout: { pattern } }] }
            }
        });
        expect(JoynrLogger.setFormatting).toHaveBeenCalledWith(pattern);
    });

    it("won't set the formatting of JoynrLogger when the patternLayout is %m", () => {
        const pattern = "%m";
        LoggingManager.configure({
            configuration: { appenders: [{ PatternLayout: { pattern } }] }
        } as any);
        expect(JoynrLogger.setFormatting).not.toHaveBeenCalled();
    });

    it("sets the output of JoynrLogger when an appenderClass is registered", () => {
        function SomeObject() {
            // Do nothing
        }
        SomeObject.prototype.append = function() {
            // Do nothing
        };
        LoggingManager.configure({ appenderClasses: { someName: SomeObject } });
        expect(JoynrLogger.setOutput).toHaveBeenCalledWith(SomeObject.prototype.append);
    });

    it(`.get returns new Instance of JoynrLogger`, () => {
        const logger = LoggingManager.getLogger("test");
        expect(logger instanceof JoynrLogger).toBeTruthy();
    });

    it(`registerForLogLevelChanged cb is called upon configure`, () => {
        const spy = jest.fn();
        LoggingManager.registerForLogLevelChanged(spy);
        LoggingManager.configure({
            configuration: {
                loggers: { root: { level: JoynrLogger.LogLevel.DEBUG } }
            }
        });
        expect(spy).toHaveBeenCalledWith(JoynrLogger.LogLevel.DEBUG);
    });
});
