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

import JoynrException = require("../../../../main/js/joynr/exceptions/JoynrException");

describe(`libjoynr-js.joynr.exceptions.JoynrException`, () => {
    const detailMessage = "someDetailedMessage";
    let exception: JoynrException;
    beforeEach(() => {
        exception = new JoynrException({ detailMessage });
    });

    it(`gets serialized without any additional properties`, () => {
        expect(JSON.stringify(exception)).toEqual(
            `{"_typeName":"joynr.exceptions.JoynrException","detailMessage":"${detailMessage}"}`
        );
    });

    it(`stack property is defined`, () => {
        expect(exception.stack).toBeDefined();
    });

    it(`can be embedded as detailMessage into template strings`, () => {
        const str = `embedded exeption: ${exception}`;
        expect(str).toEqual(`embedded exeption: JoynrException: ${detailMessage}`);
    });
});
