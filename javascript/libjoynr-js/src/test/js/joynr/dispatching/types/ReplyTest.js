/*jslint nomen: true */

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

define([
    "joynr/dispatching/types/Reply",
    "joynr/vehicle/radiotypes/RadioStation"
], function(Reply, RadioStation) {

    describe("libjoynr-js.joynr.dispatching.types.Reply", function() {

        it("is instantiable", function() {
            var response = [ "response"
            ];
            var reply = new Reply({
                requestReplyId : "id",
                response : response
            });
            expect(reply).toBeDefined();
            expect(reply instanceof Reply).toBeTruthy();
            expect(reply._typeName).toEqual("joynr.Reply");
            expect(reply.response).toEqual(response);
        });

        it("converts an untyped param to typed", function() {
            var outParameter = {
                _typeName : "joynr.vehicle.radiotypes.RadioStation",
                name : "myRadioStation"
            };
            var reply = new Reply({
                requestReplyId : "id",
                response : [ outParameter
                ]
            });
            expect(reply).toBeDefined();
            expect(reply instanceof Reply).toBeTruthy();
            expect(reply.response[0] instanceof RadioStation).toBeTruthy();
        });

    });

}); // require
/*jslint nomen: false */
