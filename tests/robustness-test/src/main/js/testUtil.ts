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

export function multipleSetImmediate(count = 10): Promise<void> {
    let res: Function;
    // eslint-disable-next-line promise/avoid-new
    const promise = new Promise<void>(resolve => (res = resolve));

    const checkCounts = (): void => (count-- > 0 ? setImmediate(checkCounts) : res());
    checkCounts();

    return promise;
}
export function reversePromise(promise: Promise<any>): Promise<any> {
    // eslint-disable-next-line promise/avoid-new
    return new Promise((resolve, reject) => {
        promise.then(reject).catch(resolve);
    });
}

export function waitForSpy(spy: jest.SpyInstance): Promise<any> {
    // eslint-disable-next-line promise/avoid-new
    return new Promise(resolve => {
        spy.mockImplementationOnce(args => resolve(args));
    });
}
