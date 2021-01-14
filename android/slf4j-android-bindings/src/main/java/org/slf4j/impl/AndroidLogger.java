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

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

import org.slf4j.Marker;
import org.slf4j.helpers.MarkerIgnoringBase;
import org.slf4j.helpers.MessageFormatter;

import android.support.annotation.IntDef;
import android.util.Log;

/**
 * A simple implementation that delegates all log requests to the Google Android
 * logging facilities. Note that this logger does not support {@link Marker}.
 * That is, methods taking marker data simply invoke the corresponding method
 * without the Marker argument, discarding any marker data passed as argument.
 * <p>
 * The logging levels specified for SLF4J can be almost directly mapped to
 * the levels that exist in the Google Android platform. The following table
 * shows the mapping implemented by this logger.
 * </p>
 * <p>
 * <table border="1" summary="">
 * 	<tr><th><b>SLF4J</b></th><th><b>Android</b></th></tr>
 * 	<tr><td>TRACE</td><td>{@link Log#VERBOSE}</td></tr>
 * 	<tr><td>DEBUG</td><td>{@link Log#DEBUG}</td></tr>
 * 	<tr><td>INFO</td><td>{@link Log#INFO}</td></tr>
 * 	<tr><td>WARN</td><td>{@link Log#WARN}</td></tr>
 * 	<tr><td>ERROR</td><td>{@link Log#ERROR}</td></tr>
 * </table>
 *
 * @author Thorsten M&ouml;ller
 * @author CTW
 */
public class AndroidLogger extends MarkerIgnoringBase {
    private static final long serialVersionUID = -1227274521521287937L;
    // The log level to be used. Default is WARN.
    @LogLevel
    private int mLogLevel = LogLevel.WARN;

    /**
     * Package access allows only {@link AndroidLoggerFactory} to instantiate
     * SimpleLogger instances.
     */
    AndroidLogger(final String name) {
        this.name = name;
    }

    int getLogLevel() {
        return mLogLevel;
    }

    void setLogLevel(@LogLevel final int logLevel) {
        mLogLevel = logLevel;
    }

    /* @see org.slf4j.Logger#isTraceEnabled() */
    @Override
    public boolean isTraceEnabled() {
        return mLogLevel <= Log.VERBOSE;
    }

    /* @see org.slf4j.Logger#trace(java.lang.String) */
    @Override
    public void trace(final String msg) {
        if (isTraceEnabled()) {
            Log.v(name, msg);
        }
    }

    /* @see org.slf4j.Logger#trace(java.lang.String, java.lang.Object) */
    @Override
    public void trace(final String format, final Object param1) {
        if (isTraceEnabled()) {
            Log.v(name, format(format, param1, null));
        }
    }

    /* @see org.slf4j.Logger#trace(java.lang.String, java.lang.Object, java.lang.Object) */
    @Override
    public void trace(final String format, final Object param1, final Object param2) {
        if (isTraceEnabled()) {
            Log.v(name, format(format, param1, param2));
        }
    }

    /* @see org.slf4j.Logger#trace(java.lang.String, java.lang.Object[]) */
    @Override
    public void trace(final String format, final Object... arguments) {
        if (isTraceEnabled()) {
            Log.v(name, format(format, arguments));
        }
    }

    /* @see org.slf4j.Logger#trace(java.lang.String, java.lang.Throwable) */
    @Override
    public void trace(final String msg, final Throwable t) {
        if (isTraceEnabled()) {
            Log.v(name, msg, t);
        }
    }

    /* @see org.slf4j.Logger#isDebugEnabled() */
    @Override
    public boolean isDebugEnabled() {
        return mLogLevel <= Log.DEBUG;
    }

    /* @see org.slf4j.Logger#debug(java.lang.String) */
    @Override
    public void debug(final String msg) {
        if (isDebugEnabled()) {
            Log.d(name, msg);
        }
    }

    /* @see org.slf4j.Logger#debug(java.lang.String, java.lang.Object) */
    @Override
    public void debug(final String format, final Object arg1) {
        if (isDebugEnabled()) {
            Log.d(name, format(format, arg1, null));
        }
    }

    /* @see org.slf4j.Logger#debug(java.lang.String, java.lang.Object, java.lang.Object) */
    @Override
    public void debug(final String format, final Object param1, final Object param2) {
        if (isDebugEnabled()) {
            Log.d(name, format(format, param1, param2));
        }
    }

    /* @see org.slf4j.Logger#debug(java.lang.String, java.lang.Object[]) */
    @Override
    public void debug(final String format, final Object... arguments) {
        if (isDebugEnabled()) {
            Log.d(name, format(format, arguments));
        }
    }

    /* @see org.slf4j.Logger#debug(java.lang.String, java.lang.Throwable) */
    @Override
    public void debug(final String msg, final Throwable t) {
        if (isDebugEnabled()) {
            Log.d(name, msg, t);
        }
    }

    /* @see org.slf4j.Logger#isInfoEnabled() */
    @Override
    public boolean isInfoEnabled() {
        return mLogLevel <= Log.INFO;
    }

    /* @see org.slf4j.Logger#info(java.lang.String) */
    @Override
    public void info(final String msg) {
        if (isInfoEnabled()) {
            Log.i(name, msg);
        }
    }

    /* @see org.slf4j.Logger#info(java.lang.String, java.lang.Object) */
    @Override
    public void info(final String format, final Object arg) {
        if (isInfoEnabled()) {
            Log.i(name, format(format, arg, null));
        }
    }

    /* @see org.slf4j.Logger#info(java.lang.String, java.lang.Object, java.lang.Object) */
    @Override
    public void info(final String format, final Object arg1, final Object arg2) {
        if (isInfoEnabled()) {
            Log.i(name, format(format, arg1, arg2));
        }
    }

    /* @see org.slf4j.Logger#info(java.lang.String, java.lang.Object[]) */
    @Override
    public void info(final String format, final Object... arguments) {
        if (isInfoEnabled()) {
            Log.i(name, format(format, arguments));
        }
    }

    /* @see org.slf4j.Logger#info(java.lang.String, java.lang.Throwable) */
    @Override
    public void info(final String msg, final Throwable t) {
        if (isInfoEnabled()) {
            Log.i(name, msg, t);
        }
    }

    /* @see org.slf4j.Logger#isWarnEnabled() */
    @Override
    public boolean isWarnEnabled() {
        return mLogLevel <= Log.WARN;
    }

    /* @see org.slf4j.Logger#warn(java.lang.String) */
    @Override
    public void warn(final String msg) {
        if (isWarnEnabled()) {
            Log.w(name, msg);
        }
    }

    /* @see org.slf4j.Logger#warn(java.lang.String, java.lang.Object) */
    @Override
    public void warn(final String format, final Object arg) {
        if (isWarnEnabled()) {
            Log.w(name, format(format, arg, null));
        }
    }

    /* @see org.slf4j.Logger#warn(java.lang.String, java.lang.Object, java.lang.Object) */
    @Override
    public void warn(final String format, final Object arg1, final Object arg2) {
        if (isWarnEnabled()) {
            Log.w(name, format(format, arg1, arg2));
        }
    }

    /* @see org.slf4j.Logger#warn(java.lang.String, java.lang.Object[]) */
    @Override
    public void warn(final String format, final Object... arguments) {
        if (isWarnEnabled()) {
            Log.w(name, format(format, arguments));
        }
    }

    /* @see org.slf4j.Logger#warn(java.lang.String, java.lang.Throwable) */
    @Override
    public void warn(final String msg, final Throwable t) {
        if (isWarnEnabled()) {
            Log.w(name, msg, t);
        }
    }

    /* @see org.slf4j.Logger#isErrorEnabled() */
    @Override
    public boolean isErrorEnabled() {
        return mLogLevel <= Log.ERROR;
    }

    /* @see org.slf4j.Logger#error(java.lang.String) */
    @Override
    public void error(final String msg) {
        if (isErrorEnabled()) {
            Log.e(name, msg);
        }
    }

    /* @see org.slf4j.Logger#error(java.lang.String, java.lang.Object) */
    @Override
    public void error(final String format, final Object arg) {
        if (isErrorEnabled()) {
            Log.e(name, format(format, arg, null));
        }
    }

    /* @see org.slf4j.Logger#error(java.lang.String, java.lang.Object, java.lang.Object) */
    @Override
    public void error(final String format, final Object arg1, final Object arg2) {
        if (isErrorEnabled()) {
            Log.e(name, format(format, arg1, arg2));
        }
    }

    /* @see org.slf4j.Logger#error(java.lang.String, java.lang.Object[]) */
    @Override
    public void error(final String format, final Object... arguments) {
        if (isErrorEnabled()) {
            Log.e(name, format(format, arguments));
        }
    }

    /* @see org.slf4j.Logger#error(java.lang.String, java.lang.Throwable) */
    @Override
    public void error(final String msg, final Throwable t) {
        if (isErrorEnabled()) {
            Log.e(name, msg, t);
        }
    }

    /**
     * For formatted messages substitute arguments.
     *
     * @param format
     * @param arg1
     * @param arg2
     */
    private String format(final String format, final Object arg1, final Object arg2) {
        return MessageFormatter.format(format, arg1, arg2).getMessage();
    }

    /**
     * For formatted messages substitute arguments.
     *
     * @param format
     * @param args
     */
    private String format(final String format, final Object[] args) {
        return MessageFormatter.arrayFormat(format, args).getMessage();
    }

    @IntDef({ LogLevel.VERBOSE, LogLevel.DEBUG, LogLevel.INFO, LogLevel.WARN, LogLevel.ERROR, LogLevel.OFF })
    @Retention(RetentionPolicy.SOURCE)
    public @interface LogLevel {
        int VERBOSE = 2;
        int DEBUG = 3;
        int INFO = 4;
        int WARN = 5;
        int ERROR = 6;
        int OFF = 7;
    }

}
