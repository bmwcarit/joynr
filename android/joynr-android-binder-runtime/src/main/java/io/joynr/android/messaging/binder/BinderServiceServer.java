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
 * Class that binds to a particular type of {@link android.app.Service}, the server's. Think of it
 * as the server in a client-server relationship, which in joynr is the CC. This
 * {@link android.app.Service} runs in the system user, which is
 * {@value io.joynr.android.messaging.binder.util.BinderConstants#USER_ID_SYSTEM}, also important
 * to know if you are running within a multi-user system.
 */
class BinderServiceServer extends BinderServiceSkeleton {

    public BinderServiceServer(Context context, Intent intent, ServiceConnection connection) {
        super(context, intent, connection);
    }

    @Override
    protected void bindService() {
        context.bindService(intent, connection, Context.BIND_AUTO_CREATE);
    }
}
