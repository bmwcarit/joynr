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

const MulticastWildcardRegexFactory = require("joynr/joynr/messaging/util/MulticastWildcardRegexFactory");

describe("libjoynr-js.joynr.messaging.util.MulticastWildcardRegexFactory", () => {
    let multicastWildcardRegexFactory;
    beforeEach(() => {
        multicastWildcardRegexFactory = new MulticastWildcardRegexFactory();
    });
    function createPattern(multicastId) {
        return new RegExp(multicastWildcardRegexFactory.createIdPattern(multicastId));
    }
    describe("createIdPattern", () => {
        function match(string, pattern) {
            return string !== undefined && string.match(pattern) !== null;
        }
        describe("works correctly for partitions without wildcards", () => {
            it("correctly specified", () => {
                const multicastId = "a/b/c";
                const pattern = createPattern(multicastId);
                expect(match(multicastId, pattern)).toEqual(true);
                expect(match("a/b", pattern)).toEqual(false);
                expect(match("a", pattern)).toEqual(false);
                expect(match("a/b/c/d", pattern)).toEqual(false);
                expect(match("b/a/b/c/d", pattern)).toEqual(false);
                expect(match("b/a/b/c", pattern)).toEqual(false);
            });
        });
        describe("works correctly for partitions with wildcards", () => {
            describe("having asterisks sign", () => {
                it("at the end", () => {
                    const multicastId = "a/b/*";
                    const pattern = createPattern(multicastId);
                    expect(match("a/b/c/d/e/f", pattern)).toEqual(true);
                    expect(match("a/b/c/d/e", pattern)).toEqual(true);
                    expect(match("a/b/c/d", pattern)).toEqual(true);
                    expect(match("a/b/c", pattern)).toEqual(true);
                    expect(match("a/b", pattern)).toEqual(true);
                    expect(match("a/bc", pattern)).toEqual(false);
                    expect(match("a/c", pattern)).toEqual(false);
                    expect(match("a", pattern)).toEqual(false);
                    expect(match("b", pattern)).toEqual(false);
                    expect(match("b/a", pattern)).toEqual(false);
                    expect(match("b/a/b", pattern)).toEqual(false);
                    expect(match("b/a/b/c", pattern)).toEqual(false);
                });
            });
            describe("having plus sign", () => {
                it("at the beginning", () => {
                    //the first two elements are ignored, as they are providerParticipantId + multicastName
                    const multicastId = "a/b/+/d/e";
                    const pattern = createPattern(multicastId);
                    expect(match("a/b/c/d/e", pattern)).toEqual(true);
                    expect(match("a/b/a/d/e", pattern)).toEqual(true);
                    expect(match("a/b/012345/d/e", pattern)).toEqual(true);
                    expect(match("a/b/c/d", pattern)).toEqual(false);
                    expect(match("a/b/c", pattern)).toEqual(false);
                    expect(match("a/b", pattern)).toEqual(false);
                    expect(match("b/a/b", pattern)).toEqual(false);
                    expect(match("b/a/b/c/d/e", pattern)).toEqual(false);
                });
                it("in the middle", () => {
                    //the first two elements are ignored, as they are providerParticipantId + multicastName
                    const multicastId = "a/b/c/+/e";
                    const pattern = createPattern(multicastId);
                    expect(match("a/b/c/d/e", pattern)).toEqual(true);
                    expect(match("a/b/c/a/e", pattern)).toEqual(true);
                    expect(match("a/b/c/012345/e", pattern)).toEqual(true);
                    expect(match("a/b/c/012345", pattern)).toEqual(false);
                    expect(match("a/b/c/d/e/f", pattern)).toEqual(false);
                });
                it("at the end", () => {
                    const multicastId = "a/b/c/d/+";
                    const pattern = createPattern(multicastId);
                    expect(match("a/b/c/d/e", pattern)).toEqual(true);
                    expect(match("a/b/c/d/x", pattern)).toEqual(true);
                    expect(match("a/b/c/d/012345", pattern)).toEqual(true);
                    expect(match("a/b/c/d", pattern)).toEqual(false);
                    expect(match("a/b/c/d/e/x", pattern)).toEqual(false);
                });
                it("multiple times", () => {
                    const multicastId = "a/b/c/+/e/+";
                    const pattern = createPattern(multicastId);
                    expect(match("a/b/c/d/e/f", pattern)).toEqual(true);
                    expect(match("a/b/c/a/e/xyz", pattern)).toEqual(true);
                    expect(match("a/b/c", pattern)).toEqual(false);
                    expect(match("a/b/c/d", pattern)).toEqual(false);
                    expect(match("a/b/c/d/e/f/g", pattern)).toEqual(false);
                });
            });
            describe("having mixed plus and asterisk sign", () => {
                it("correctly specified", () => {
                    const multicastId = "a/b/+/d/*";
                    const pattern = createPattern(multicastId);
                    expect(match("a/b/c/d/e", pattern)).toEqual(true);
                    expect(match("a/b/x/d/e/f/g", pattern)).toEqual(true);
                });
            });
        });
    });
    describe("compare regular expressions", () => {
        describe("with same multicastId as input", () => {
            let pattern, pattern2;
            beforeEach(() => {
                const multicastId = "a/b/c";
                pattern = createPattern(multicastId);
                pattern2 = createPattern(multicastId);
            });
            it("and test equal operator", () => {
                expect(pattern).toEqual(pattern2);
            });
            it("and pattern instance as object key", () => {
                let testValue;
                const x = {};
                x[pattern] = testValue;
                expect(x[pattern2]).toBe(testValue);
            });
        });
        it("with different multicastId as input", () => {
            const pattern = createPattern("a/b/c");
            const pattern2 = createPattern("a/b");
            expect(pattern).not.toEqual(pattern2);
        });
    });
});
