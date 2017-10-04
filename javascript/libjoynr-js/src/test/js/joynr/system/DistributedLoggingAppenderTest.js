/*jslint node: true */

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
var LoggingManager = require('../../../classes/joynr/system/LoggingManager');
var DistributedLoggingAppender =
        require('../../../classes/joynr/system/DistributedLoggingAppender');
var loggingProxy;

loggingProxy = {
    log : function(value) {},
    providerDiscoveryEntry : {}
};

describe("libjoynr-js.joynr.system.DistributedLoggingAppender", function() {

    beforeEach(function(done) {
        jasmine.clock().install();
        done();
    });

    afterEach(function(done) {
        jasmine.clock().uninstall();
        done();
    });

    it("is instantiable", function(done) {
        expect(new DistributedLoggingAppender()).toBeDefined();
        done();
    });

    it("uses the correct interval", function(done) {
        var config, context, appender, loggingEvent;

        context = {};
        config = {
            flushInterval : 1000,
            flushMaxLogEventsCount : 3
        };

        spyOn(loggingProxy, "log");

        appender = new DistributedLoggingAppender(config, context);
        appender.setProxy(loggingProxy);

        expect(loggingProxy.log).not.toHaveBeenCalled();

        loggingEvent = {
            logger : {
                name : "loggerName"
            },
            timeStampInMilliseconds : 10000000,
            messages : [ "log message"
            ],
            level : {
                name : "DEBUG"
            }
        };
        appender.append(loggingEvent);

        expect(loggingProxy.log).not.toHaveBeenCalled();

        jasmine.clock().tick(1000);

        expect(loggingProxy.log).toHaveBeenCalled();
        done();
    });

    it("flushes when the max number of events has been reached", function(done) {
        var config, context, appender, loggingEvent;

        context = {};
        config = {
            flushInterval : 1000,
            flushMaxLogEventsCount : 3

        };

        spyOn(loggingProxy, "log");

        appender = new DistributedLoggingAppender(config, context);
        appender.setProxy(loggingProxy);

        expect(loggingProxy.log).not.toHaveBeenCalled();
        loggingEvent = {
            logger : {
                name : "loggerName"
            },
            timeStampInMilliseconds : 10000000,
            messages : [ "log message"
            ],
            level : {
                name : "DEBUG"
            }
        };
        appender.append(loggingEvent);
        expect(loggingProxy.log).not.toHaveBeenCalled();

        appender.append(loggingEvent);
        expect(loggingProxy.log).not.toHaveBeenCalled();

        appender.append(loggingEvent);
        expect(loggingProxy.log).toHaveBeenCalled();
        done();
    });

});
