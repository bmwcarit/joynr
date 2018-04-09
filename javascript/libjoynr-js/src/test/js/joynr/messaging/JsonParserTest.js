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

define(["JsonParser", "joynr/system/LoggingManager"], (JsonParser, LoggingManager) => {
    describe("libjoynr-js.joynr.messaging.JsonParserTest", () => {
        const log = LoggingManager.getLogger("JsonParserTest");

        it("testSimpleObject", () => {
            const object1 = {
                x: "xü/"
            };

            const json = JSON.stringify(object1);
            const jsonParser = new JsonParser(json);
            const object1result = jsonParser.next;

            expect(object1).toEqual(object1result);
        });

        it("testParseMultipleObjects", () => {
            const object1 = {
                x: "x"
            };
            const object2 = {
                y: "y"
            };

            const concatenatedJson = JSON.stringify(object1) + JSON.stringify(object2);
            const jsonParser = new JsonParser(concatenatedJson);
            const object1result = jsonParser.next;

            expect(object1).toEqual(object1result);
        });

        it("testParseMultipleComplexObjects", () => {
            const objs = [
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
            let i;
            let concatenatedJson = "";
            for (i = 0; i < objs.length; i++) {
                concatenatedJson += JSON.stringify(objs[i]);
            }

            const jsonParser = new JsonParser(concatenatedJson);
            let x = 0;
            while (jsonParser.hasNext) {
                const next = jsonParser.next;
                expect(objs[x]).toEqual(next);
                x++;
            }
            expect(x).toEqual(objs.length);
        });

        it("testParseInvalidObjects", () => {
            const object1 = {
                x: "x"
            };
            const object2 = "a";

            const concatenatedJson = JSON.stringify(object1) + JSON.stringify(object2);
            const jsonParser = new JsonParser(concatenatedJson);

            let next = null;
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
