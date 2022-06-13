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
/*eslint no-unused-vars: "off"*/
const noop = function(): any {
    // do nothing
};

export type LogLevel = "trace" | "debug" | "info" | "warn" | "error" | "fatal" | "off";
type LoggingFunction = (message: string, object?: any) => void;
type LoggingFormatSteps = (output: string, settings: LoggingSettings) => string;

interface LoggingSettings {
    message: string;
    level: LogLevel;
    name: string;
}

export interface LoggingEvent {
    level: { name: string };
    messages: string[];
}

/**
 * JoynrLogger can be instantiated before any settings are done. The settings will later on be set
 * only once for JoynrLogger, which will then apply it for all members. This process is being done
 * by editing the prototype. A factory pattern would do this potentially in a cleaner way, but this
 * class was heavily optimized for performance and any changes would bring more overhead.
 *
 * Typescript doesn't allow access direct access to the prototype and doesn't understand how the
 * prototype is manipulated. Thus there are multiple @ts-ignore statements.
 */
export default class JoynrLogger {
    public static LogLevel: Record<string, LogLevel> = {
        TRACE: "trace",
        DEBUG: "debug",
        INFO: "info",
        WARN: "warn",
        ERROR: "error",
        FATAL: "fatal",
        OFF: "off"
    };

    private name: string;

    public trace!: LoggingFunction;
    public debug!: LoggingFunction;
    public info!: LoggingFunction;
    public warn!: LoggingFunction;
    public error!: LoggingFunction;
    public fatal!: LoggingFunction;

    private static trace: LoggingFunction = JoynrLogger.createLog(JoynrLogger.LogLevel.TRACE);
    private static debug: LoggingFunction = JoynrLogger.createLog(JoynrLogger.LogLevel.DEBUG);
    private static info: LoggingFunction = JoynrLogger.createLog(JoynrLogger.LogLevel.INFO);
    private static warn: LoggingFunction = JoynrLogger.createLog(JoynrLogger.LogLevel.WARN);
    private static error: LoggingFunction = JoynrLogger.createLog(JoynrLogger.LogLevel.ERROR);
    private static fatal: LoggingFunction = JoynrLogger.createLog(JoynrLogger.LogLevel.FATAL);

    private static level: LogLevel = "off";
    private static output: Function = noop;

    private log!: Function;
    private format?: Function;

    public constructor(name = "root") {
        this.name = name;
    }

    public isDebugEnabled(): boolean {
        return JoynrLogger.level === JoynrLogger.LogLevel.DEBUG || JoynrLogger.level === JoynrLogger.LogLevel.TRACE;
    }

    public static setLogLevel(level: LogLevel): void {
        JoynrLogger.prototype.trace = noop;
        JoynrLogger.prototype.debug = noop;
        JoynrLogger.prototype.info = noop;
        JoynrLogger.prototype.warn = noop;
        JoynrLogger.prototype.error = noop;
        JoynrLogger.prototype.fatal = noop;

        /* eslint-disable no-fallthrough */
        switch (level) {
            case JoynrLogger.LogLevel.TRACE:
                JoynrLogger.prototype.trace = JoynrLogger.trace;
            case JoynrLogger.LogLevel.DEBUG:
                JoynrLogger.prototype.debug = JoynrLogger.debug;
            case JoynrLogger.LogLevel.INFO:
                JoynrLogger.prototype.info = JoynrLogger.info;
            case JoynrLogger.LogLevel.WARN:
                JoynrLogger.prototype.warn = JoynrLogger.warn;
            case JoynrLogger.LogLevel.ERROR:
                JoynrLogger.prototype.error = JoynrLogger.error;
            case JoynrLogger.LogLevel.FATAL:
                JoynrLogger.prototype.fatal = JoynrLogger.fatal;
                break;
            case JoynrLogger.LogLevel.OFF:
                break;
            default:
                throw new Error(`invalid log level ${level}`);
        }
        /* eslint-enable no-fallthrough */
        JoynrLogger.level = level;
    }

    private static logWithoutFormatting(this: JoynrLogger, message: string, level: LogLevel): void {
        JoynrLogger.output({ level: { name: level }, messages: [message] });
    }

    private static logWithFormatting(this: JoynrLogger, message: string, level: LogLevel): void {
        // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
        const formattedMessage = this.format!(message, level);
        JoynrLogger.output({ level: { name: level }, messages: [formattedMessage] });
    }

    public static setOutput(outputFunction: Function): void {
        JoynrLogger.output = outputFunction;
    }

    public static setFormatting(pattern: string): void {
        const steps = [];
        /* pattern : "[%d{HH:mm:ss,SSS}][%c][%p] %m{2}"*/
        function checkRest(restString: string): void {
            if (restString !== "") {
                steps.push(curryAddString(restString));
            }
        }

        const regexPattern = new RegExp("{[^}]*}", "g");
        // too complicated patterns are not supported
        const patternWithoutArguments = pattern.replace(regexPattern, "");

        const splitString = patternWithoutArguments.split("%");
        const firstElement = splitString.shift();
        if (firstElement) {
            steps.push(curryAddString(firstElement));
        }
        while (splitString.length) {
            let subString = splitString.shift() as string;
            const command = subString.charAt(0);
            subString = subString.slice(1);
            switch (command) {
                case "d":
                    steps.push(addDate);
                    checkRest(subString);
                    break;
                case "c": // logger name
                    steps.push(addLoggerName);
                    checkRest(subString);
                    break;
                case "p": // log level
                    steps.push(addLogLevel);
                    checkRest(subString);
                    break;
                case "m": // message
                    steps.push(addMessage);
                    checkRest(subString);
                    break;
                default:
                    break;
            }
        }

        JoynrLogger.prototype.format = JoynrLogger.createFormattingFunctionFromSteps(steps);
        JoynrLogger.prototype.log = JoynrLogger.logWithFormatting;
    }

    private static createFormattingFunctionFromSteps(
        steps: LoggingFormatSteps[]
    ): (message: string, level: LogLevel) => string {
        return function(this: JoynrLogger, message: string, level: LogLevel): string {
            let i,
                output = "";
            const length = steps.length;
            const settings = {
                message,
                level,
                name: this.name
            };
            for (i = 0; i < length; i++) {
                output = steps[i](output, settings);
            }
            return output;
        };
    }

    private static createLog(level: LogLevel): LoggingFunction {
        return function(this: JoynrLogger, message: string, object: any): void {
            if (object) {
                try {
                    message += ` ${JSON.stringify(object)}`;
                } catch (e) {}
            }
            this.log(message, level);
        };
    }
}

JoynrLogger.prototype.trace = noop;
JoynrLogger.prototype.debug = noop;
JoynrLogger.prototype.info = noop;
JoynrLogger.prototype.warn = noop;
JoynrLogger.prototype.error = noop;
JoynrLogger.prototype.fatal = noop;
// @ts-ignore
JoynrLogger.prototype.log = JoynrLogger.logWithoutFormatting;

function curryAddString(string: string): LoggingFormatSteps {
    // eslint-disable-next-line @typescript-eslint/no-unused-vars
    return (output: string, _settings: LoggingSettings): string => output + string;
}

// eslint-disable-next-line @typescript-eslint/no-unused-vars
function addDate(output: string, _settings: LoggingSettings): string {
    const date = new Date();
    return `${output + date.getHours()}:${date.getMinutes()}:${date.getSeconds()},${date.getMilliseconds()}`;
}

function addLoggerName(output: string, settings: LoggingSettings): string {
    return output + settings.name;
}

function addLogLevel(output: string, settings: LoggingSettings): string {
    return output + settings.level;
}

function addMessage(output: string, settings: LoggingSettings): string {
    return output + settings.message;
}
