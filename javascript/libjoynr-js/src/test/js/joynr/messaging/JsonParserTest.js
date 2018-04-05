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

define(["JsonParser", "joynr/system/LoggingManager"], function(JsonParser, LoggingManager) {
    describe("libjoynr-js.joynr.messaging.JsonParserTest", function() {
        var log = LoggingManager.getLogger("JsonParserTest");

        it("testSimpleObject", function() {
            var object1 = {
                x: "xü/"
            };

            var json = JSON.stringify(object1);
            var jsonParser = new JsonParser(json);
            var object1result = jsonParser.next;

            expect(object1).toEqual(object1result);
        });

        it("testParseMultipleObjects", function() {
            var object1 = {
                x: "x"
            };
            var object2 = {
                y: "y"
            };

            var concatenatedJson = JSON.stringify(object1) + JSON.stringify(object2);
            var jsonParser = new JsonParser(concatenatedJson);
            var object1result = jsonParser.next;

            expect(object1).toEqual(object1result);
        });

        it("testParseMultipleComplexObjects", function() {
            var objs = [
                {
                    dog: "dog",
                    cat: "cat",
                    mouse: {
                        giraffe: "tall",
                        pigeon: {
                            snake: "(/Z(bjkljbslkjw78sjk()?H(2ä#"
                        },
                        snail: [
                            {
                                a: "slow"
                            }
                        ]
                    },
                    lion: "roar",
                    zoo: [1, 2, 3, 4, 5, 6, 7, 8, 9]
                },
                {
                    car: "BMW",
                    truck: "big",
                    plane: {
                        wings: 2.0
                    }
                },
                {
                    red: "red",
                    green: "green",
                    yellow: {
                        purple: "purple"
                    }
                },
                {
                    1: 1,
                    2: 2,
                    3: {
                        33: "33333333333333",
                        44: "4444444444444444"
                    }
                }
            ];
            var i;
            var concatenatedJson = "";
            for (i = 0; i < objs.length; i++) {
                concatenatedJson += JSON.stringify(objs[i]);
            }

            var jsonParser = new JsonParser(concatenatedJson);
            var x = 0;
            while (jsonParser.hasNext) {
                var next = jsonParser.next;
                expect(objs[x]).toEqual(next);
                x++;
            }
            expect(x).toEqual(objs.length);
        });

        it("testParseInvalidObjects", function() {
            var object1 = {
                x: "x"
            };
            var object2 = "a";

            var concatenatedJson = JSON.stringify(object1) + JSON.stringify(object2);
            var jsonParser = new JsonParser(concatenatedJson);

            var next = null;
            // TODO this should work with:
            // assertException(jsonParser.next, "SyntaxError");
            try {
                next = jsonParser.next;
            } catch (e) {
                expect("SyntaxError").toEqual(e.name);
            }
            expect(next).toBeNull();
        });
    });
});
