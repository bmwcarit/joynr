/*global joynrTestRequire: true */

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

joynrTestRequire("joynr/messaging/routing/TestLocalChannelUrlDirectory", [
    "joynr/types/ChannelUrlInformation",
    "joynr/messaging/routing/LocalChannelUrlDirectory",
    "global/Promise",
    "Date"
], function(ChannelUrlInformation, LocalChannelUrlDirectory, Promise, Date) {

    var fakeTime = 0;

    function increaseFakeTime(time_ms) {
        fakeTime = fakeTime + time_ms;
        jasmine.Clock.tick(time_ms);
    }

    describe("libjoynr-js.joynr.messaging.routing.LocalChannelUrlDirectory", function() {
        var localChannelUrlDirectory;
        var globalChannelUrlDirectoryProxy;
        var channelId;
        var channelUrlInformation;
        var operationArguments;
        var freshness;
        var channelUrlDirectoryChannelId;
        var channelUrlDirectoryChannelUrl;
        var capabilitiesDirectoryChannelId;
        var capabilitiesDirectoryChannelUrl;
        var provisionedChannelUrls;

        beforeEach(function() {
            channelId = "channelId";
            channelUrlInformation = new ChannelUrlInformation({
                urls : [
                    "url1",
                    "url2"
                ]
            });
            operationArguments = {
                channelId : channelId,
                channelUrlInformation : channelUrlInformation
            };
            freshness = 10 * 1000; // 10 seconds

            globalChannelUrlDirectoryProxy = jasmine.createSpyObj("globalChannelUrlDirectory", [
                "registerChannelUrls",
                "unregisterChannelUrls",
                "getUrlsForChannel"
            ]);
            globalChannelUrlDirectoryProxy.registerChannelUrls.andReturn(Promise.resolve());
            globalChannelUrlDirectoryProxy.unregisterChannelUrls.andReturn(Promise.resolve());
            globalChannelUrlDirectoryProxy.getUrlsForChannel.andReturn(Promise.resolve({}));

            channelUrlDirectoryChannelId = "channelUrlDirectoryChannelId";
            channelUrlDirectoryChannelUrl = "channelUrlDirectoryChannelUrl";
            capabilitiesDirectoryChannelId = "capabilitiesDirectoryChannelId";
            capabilitiesDirectoryChannelUrl = "capabilitiesDirectoryChannelUrl";

            provisionedChannelUrls = {};
            provisionedChannelUrls[channelUrlDirectoryChannelId] =
                    new ChannelUrlInformation([ channelUrlDirectoryChannelUrl
                    ]);
            provisionedChannelUrls[capabilitiesDirectoryChannelId] =
                    new ChannelUrlInformation([ capabilitiesDirectoryChannelUrl
                    ]);

            localChannelUrlDirectory = new LocalChannelUrlDirectory({
                channelUrlDirectoryProxy : globalChannelUrlDirectoryProxy,
                provisionedChannelUrls : provisionedChannelUrls
            });

            fakeTime = Date.now();
            jasmine.Clock.useMock();
            jasmine.Clock.reset();
            spyOn(Date, "now").andCallFake(function() {
                return fakeTime;
            });
        });

        it("is instantiable", function() {
            expect(localChannelUrlDirectory).toBeDefined();
        });

        it("getUrlsForChannel calls through to global capabilities directory", function() {
            runs(function() {
                localChannelUrlDirectory.getUrlsForChannel(operationArguments);
                increaseFakeTime(1);
            });

            waitsFor(function() {
                return globalChannelUrlDirectoryProxy.getUrlsForChannel.callCount > 0;
            }, 100);

            runs(function() {
                expect(globalChannelUrlDirectoryProxy.getUrlsForChannel).toHaveBeenCalled();
                expect(globalChannelUrlDirectoryProxy.getUrlsForChannel).toHaveBeenCalledWith(
                        operationArguments);
            });
        });

        it("registersChannelUrls calls through to global capabilities directory", function() {
            runs(function() {
                localChannelUrlDirectory.registerChannelUrls(operationArguments);
                increaseFakeTime(1);
            });

            waitsFor(function() {
                return globalChannelUrlDirectoryProxy.registerChannelUrls.callCount > 0;
            }, 100);

            runs(function() {
                expect(globalChannelUrlDirectoryProxy.registerChannelUrls).toHaveBeenCalled();
                expect(globalChannelUrlDirectoryProxy.registerChannelUrls).toHaveBeenCalledWith(
                        operationArguments);
            });
        });

        it("unregisterChannelUrls calls through to global capabilities directory", function() {
            runs(function() {
                localChannelUrlDirectory.unregisterChannelUrls(operationArguments);
                increaseFakeTime(1);
            });

            waitsFor(function() {
                return globalChannelUrlDirectoryProxy.unregisterChannelUrls.callCount > 0;
            }, 100);

            runs(function() {
                expect(globalChannelUrlDirectoryProxy.unregisterChannelUrls).toHaveBeenCalled();
                expect(globalChannelUrlDirectoryProxy.unregisterChannelUrls).toHaveBeenCalledWith(
                        operationArguments);
            });
        });

        function testProvisioned(channelId, channelUrl) {
            var fulfilledSpy;
            runs(function() {
                fulfilledSpy = jasmine.createSpy("fulfilledSpy");
                localChannelUrlDirectory.getUrlsForChannel({
                    channelId : channelId
                }).then(fulfilledSpy);
                increaseFakeTime(1);
            });

            waitsFor(function() {
                return fulfilledSpy.callCount > 0;
            }, 100);

            runs(function() {
                expect(globalChannelUrlDirectoryProxy.getUrlsForChannel).not.toHaveBeenCalled();
                expect(fulfilledSpy).toHaveBeenCalled();
                expect(fulfilledSpy).toHaveBeenCalledWith(new ChannelUrlInformation([ channelUrl
                ]));
            });
        }

        it("retrieves provisioned channel URLs of channelUrlDirectory correctly", function() {
            testProvisioned(channelUrlDirectoryChannelId, channelUrlDirectoryChannelUrl);
        });

        it("retrieves provisioned channel URLs of capabilitiesDirectory correctly", function() {
            testProvisioned(capabilitiesDirectoryChannelId, capabilitiesDirectoryChannelUrl);
        });

        it("registersChannelUrls registers urls for the given channelId", function() {
            var fulfilledSpy = jasmine.createSpy("fulfilledSpy");

            runs(function() {
                localChannelUrlDirectory.registerChannelUrls(operationArguments);
                localChannelUrlDirectory.getUrlsForChannel({
                    channelId : channelId
                }).then(fulfilledSpy);
                increaseFakeTime(1);
            });

            waitsFor(function() {
                return fulfilledSpy.callCount > 0;
            }, 100);

            runs(function() {
                expect(fulfilledSpy).toHaveBeenCalled();
                expect(fulfilledSpy).toHaveBeenCalledWith(channelUrlInformation);
            });
        });

        it("caches urls locally", function() {
            var fulfilledSpy = jasmine.createSpy("fulfilledSpy");
            globalChannelUrlDirectoryProxy.getUrlsForChannel.andReturn(Promise
                    .resolve(channelUrlInformation));
            runs(function() {
                localChannelUrlDirectory.getUrlsForChannel({
                    channelId : channelId
                }).then(fulfilledSpy);
                increaseFakeTime(1);
            });

            waitsFor(function() {
                return fulfilledSpy.callCount > 0;
            }, 100);

            runs(function() {
                fulfilledSpy.reset();
                expect(globalChannelUrlDirectoryProxy.getUrlsForChannel).toHaveBeenCalled();
                globalChannelUrlDirectoryProxy.getUrlsForChannel.reset();
                localChannelUrlDirectory.getUrlsForChannel({
                    channelId : channelId
                }, freshness).then(fulfilledSpy);
                increaseFakeTime(1);
            });

            waitsFor(function() {
                return fulfilledSpy.callCount > 0;
            }, 100);

            runs(function() {
                expect(globalChannelUrlDirectoryProxy.getUrlsForChannel).not.toHaveBeenCalled();
            });
        });

        it("calls the global proxy if freshness has expired", function() {
            var spy = jasmine.createSpyObj("spy", [ "fulfilled"
            ]);
            globalChannelUrlDirectoryProxy.getUrlsForChannel.andReturn(Promise
                    .resolve(channelUrlInformation));
            runs(function() {
                localChannelUrlDirectory.getUrlsForChannel({
                    channelId : channelId
                }).then(spy.fulfilled);
                increaseFakeTime(1);
            });

            waitsFor(function() {
                return spy.fulfilled.callCount > 0;
            }, 100);

            runs(function() {
                spy.fulfilled.reset();
                expect(globalChannelUrlDirectoryProxy.getUrlsForChannel).toHaveBeenCalled();
                globalChannelUrlDirectoryProxy.getUrlsForChannel.reset();

                increaseFakeTime(freshness); // set time to freshness
                localChannelUrlDirectory.getUrlsForChannel({
                    channelId : channelId
                }, freshness).then(spy.fulfilled);
                increaseFakeTime(1);
            });

            waitsFor(function() {
                return spy.fulfilled.callCount > 0;
            }, 100);

            runs(function() {
                spy.fulfilled.reset();
                expect(globalChannelUrlDirectoryProxy.getUrlsForChannel).not.toHaveBeenCalled();
                globalChannelUrlDirectoryProxy.getUrlsForChannel.reset();

                increaseFakeTime(1); // set time to freshness + 1
                localChannelUrlDirectory.getUrlsForChannel({
                    channelId : channelId
                }, freshness).then(spy.fulfilled);
                increaseFakeTime(1);
            });

            waitsFor(function() {
                return spy.fulfilled.callCount > 0;
            }, 100);

            runs(function() {
                expect(globalChannelUrlDirectoryProxy.getUrlsForChannel).toHaveBeenCalled();
            });
        });
    });
});