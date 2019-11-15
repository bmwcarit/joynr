/*-
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
/*
 * Created 21.10.2009
 *
 * Copyright (c) 2009-2012 SLF4J.ORG
 *
 * All rights reserved.
 *
 * Permission is hereby granted, free  of charge, to any person obtaining
 * a  copy  of this  software  and  associated  documentation files  (the
 * "Software"), to  deal in  the Software without  restriction, including
 * without limitation  the rights to  use, copy, modify,  merge, publish,
 * distribute,  sublicense, and/or sell  copies of  the Software,  and to
 * permit persons to whom the Software  is furnished to do so, subject to
 * the following conditions:
 *
 * The  above  copyright  notice  and  this permission  notice  shall  be
 * included in all copies or substantial portions of the Software.
 *
 * THE  SOFTWARE IS  PROVIDED  "AS  IS", WITHOUT  WARRANTY  OF ANY  KIND,
 * EXPRESS OR  IMPLIED, INCLUDING  BUT NOT LIMITED  TO THE  WARRANTIES OF
 * MERCHANTABILITY,    FITNESS    FOR    A   PARTICULAR    PURPOSE    AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE,  ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
package org.slf4j.impl;

import static org.slf4j.impl.AndroidLogger.LogLevel.DEBUG;
import static org.slf4j.impl.AndroidLogger.LogLevel.ERROR;
import static org.slf4j.impl.AndroidLogger.LogLevel.INFO;
import static org.slf4j.impl.AndroidLogger.LogLevel.OFF;
import static org.slf4j.impl.AndroidLogger.LogLevel.VERBOSE;
import static org.slf4j.impl.AndroidLogger.LogLevel.WARN;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.Locale;

import org.slf4j.ILoggerFactory;
import org.slf4j.LoggerFactory;
import org.slf4j.spi.LoggerFactoryBinder;

import android.support.annotation.StringDef;

/**
 * The binding of {@link LoggerFactory} class with an actual instance of
 * {@link ILoggerFactory} is performed using information returned by this class.
 *
 * @author Ceki G&uuml;lc&uuml;
 * @author Thorsten M&ouml;ler
 * @author CTW
 */
public class StaticLoggerBinder implements LoggerFactoryBinder {

    /**
     * The unique instance of this class.
     */
    private static final StaticLoggerBinder SINGLETON = new StaticLoggerBinder();
    /**
     * Declare the version of the SLF4J API this implementation is compiled
     * against. The value of this field is usually modified with each release.
     */
    // to avoid constant folding by the compiler, this field must *not* be final
    public static String REQUESTED_API_VERSION = "1.6.99"; // !final
    /**
     * The ILoggerFactory instance returned by the {@link #getLoggerFactory}
     * method should always be the same object
     */
    private final ILoggerFactory loggerFactory;

    private StaticLoggerBinder() {
        loggerFactory = new AndroidLoggerFactory();
    }

    /**
     * Return the singleton of this class.
     *
     * @return the StaticLoggerBinder singleton
     */
    public static StaticLoggerBinder getSingleton() {
        return SINGLETON;
    }

    /**
     * Set the log level to use for all Android logs that make use of SLF4J Android. This is one 
     * of {@link org.slf4j.impl.AndroidLogger.LogLevel}'s interface. By setting this, you will 
     * set the same level for all currently existing as well as newly created loggers.
     * 
     * @param logLevel The {@link org.slf4j.impl.AndroidLogger.LogLevel} to set.
     */
    public static void setLogLevel(@AndroidLogger.LogLevel final int logLevel) {
        ((AndroidLoggerFactory) getSingleton().getLoggerFactory()).loggerMap.forEach((k, v) -> v.setLogLevel(logLevel));
        ((AndroidLoggerFactory) getSingleton().getLoggerFactory()).setLogLevel(logLevel);
    }

    /**
     * Return a valid mapping for log level by using the 
     * {@link org.slf4j.impl.AndroidLogger.LogLevel} {@link android.support.annotation.IntDef}
     * annotation. Valid values from lower to higher priority, ignoring case:
     * <ol>
     *     <li>VERBOSE</li>
     *     <li>DEBUG</li>
     *     <li>INFO</li>
     *     <li>WARN</li>
     *     <li>ERROR</li>
     *     <li>OFF (turns logging off)</li>
     * </ol>
     *
     * @param logLevel The log level as a {@link String}. Use one of VERBOSE, DEBUG, INFO, WARN, 
     *                 ERROR and OFF.
     * @return The mapped value for use with the SLF4J Android implementation, which you can then
     * pass to {@link StaticLoggerBinder#setLogLevel(int)}. If an invalid value is passed, you 
     * will receive {@link org.slf4j.impl.AndroidLogger.LogLevel#WARN} as it is the default log 
     * level.
     */
    @AndroidLogger.LogLevel
    public static int getValidLogLevel(final String logLevel) {
        switch (logLevel.toUpperCase(Locale.ENGLISH)) {
        case LogLevelString.VERBOSE:
            return VERBOSE;
        case LogLevelString.DEBUG:
            return DEBUG;
        case LogLevelString.INFO:
            return INFO;
        case LogLevelString.WARN:
            return WARN;
        case LogLevelString.ERROR:
            return ERROR;
        case LogLevelString.OFF:
            return OFF;
        default:
            return WARN;
        }
    }

    @Override
    public ILoggerFactory getLoggerFactory() {
        return loggerFactory;
    }

    @Override
    public String getLoggerFactoryClassStr() {
        return AndroidLoggerFactory.class.getName();
    }

    @StringDef({ LogLevelString.VERBOSE, LogLevelString.DEBUG, LogLevelString.INFO, LogLevelString.WARN,
            LogLevelString.ERROR, LogLevelString.OFF })
    @Retention(RetentionPolicy.SOURCE)
    private @interface LogLevelString {
        String VERBOSE = "VERBOSE";
        String DEBUG = "DEBUG";
        String INFO = "INFO";
        String WARN = "WARN";
        String ERROR = "ERROR";
        String OFF = "OFF";
    }
}
