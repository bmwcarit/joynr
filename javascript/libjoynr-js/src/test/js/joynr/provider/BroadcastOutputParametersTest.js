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
require("../../node-unit-test-helper");
const BroadcastOutputParameters = require("../../../../main/js/joynr/provider/BroadcastOutputParameters");

describe("libjoynr-js.joynr.provider.BroadcastOutputParameters", () => {
    it("is instantiable", () => {
        expect(
            new BroadcastOutputParameters([
                {
                    name: "param1",
                    type: "String"
                },
                {
                    name: "param2",
                    type: "String"
                }
            ])
        ).toBeDefined();
    });

    it("is of correct type", () => {
        const broadcastOutputParameters = new BroadcastOutputParameters([
            {
                name: "param1",
                type: "String"
            },
            {
                name: "param2",
                type: "String"
            }
        ]);
        expect(broadcastOutputParameters).toBeDefined();
        expect(broadcastOutputParameters).not.toBeNull();
        expect(typeof broadcastOutputParameters === "object").toBeTruthy();
        expect(broadcastOutputParameters instanceof BroadcastOutputParameters).toEqual(true);
    });

    it("can be empty", () => {
        const broadcastOutputParameters = new BroadcastOutputParameters([]);
        expect(broadcastOutputParameters).toBeDefined();
        expect(broadcastOutputParameters).not.toBeNull();
        expect(typeof broadcastOutputParameters === "object").toBeTruthy();
        expect(broadcastOutputParameters instanceof BroadcastOutputParameters).toEqual(true);
    });

    it("has setters and getters for each output parameter", () => {
        const broadcastOutputParameters = new BroadcastOutputParameters([
            {
                name: "param1",
                type: "String"
            },
            {
                name: "param2",
                type: "String"
            }
        ]);
        expect(broadcastOutputParameters.setParam1).toBeDefined();
        expect(broadcastOutputParameters.setParam2).toBeDefined();
        expect(broadcastOutputParameters.getParam1).toBeDefined();
        expect(broadcastOutputParameters.getParam2).toBeDefined();
        expect(typeof broadcastOutputParameters.setParam1 === "function").toBeTruthy();
        expect(typeof broadcastOutputParameters.setParam2 === "function").toBeTruthy();
        expect(typeof broadcastOutputParameters.getParam1 === "function").toBeTruthy();
        expect(typeof broadcastOutputParameters.getParam2 === "function").toBeTruthy();
    });

    it("setters and getters store and retrieve correct values", () => {
        const broadcastOutputParameters = new BroadcastOutputParameters([
            {
                name: "param1",
                type: "String"
            },
            {
                name: "param2",
                type: "String"
            }
        ]);
        broadcastOutputParameters.setParam1("Hello");
        broadcastOutputParameters.setParam2("world");
        expect(broadcastOutputParameters.getParam1()).toEqual("Hello");
        expect(broadcastOutputParameters.getParam2()).toEqual("world");
    });

    it("array outputParameters contains correct values", () => {
        const broadcastOutputParameters = new BroadcastOutputParameters([
            {
                name: "param1",
                type: "String"
            },
            {
                name: "param2",
                type: "String"
            }
        ]);
        broadcastOutputParameters.setParam2("world");
        broadcastOutputParameters.setParam1("Hello");
        expect(broadcastOutputParameters.outputParameters[0]).toEqual("Hello");
        expect(broadcastOutputParameters.outputParameters[1]).toEqual("world");
    });
});
