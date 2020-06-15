/*-
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
package io.joynr.android.messaging.binder;

import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;

/**
 * Skeleton base class for a {@link android.app.Service} that will be bound.
 */
abstract class BinderServiceSkeleton {

    protected Context context;
    protected Intent intent;
    protected ServiceConnection connection;

    protected BinderServiceSkeleton(Context context, Intent intent, ServiceConnection connection) {
        this.context = context;
        this.intent = intent;
        this.connection = connection;
    }

    /**
     * Performs logic that binds to a {@link android.app.Service}.
     */
    protected abstract void bindService();

}
