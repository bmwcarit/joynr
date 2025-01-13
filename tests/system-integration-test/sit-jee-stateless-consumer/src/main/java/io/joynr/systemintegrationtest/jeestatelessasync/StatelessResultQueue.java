/*
 * #%L
 * %%
 * Copyright (C) 2025 BMW Car IT GmbH
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
package io.joynr.systemintegrationtest.jeestatelessasync;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import jakarta.inject.Singleton;

import io.joynr.exceptions.JoynrWaitExpiredException;

@Singleton
public class StatelessResultQueue {

    private Semaphore resultAvailableSemaphore = new Semaphore(0);
    private List<String> results = new ArrayList<>();

    public String getResult(int timeoutMs) throws InterruptedException, JoynrWaitExpiredException {
        if (resultAvailableSemaphore.tryAcquire(timeoutMs, TimeUnit.MILLISECONDS)) {
            synchronized (results) {
                return results.remove(0);
            }
        }
        throw new JoynrWaitExpiredException();
    }

    public void addResult(String result) {
        synchronized (results) {
            results.add(result);
        }
        resultAvailableSemaphore.release();
    }
}
