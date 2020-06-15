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

import io.joynr.android.messaging.binder.interfaces.IBinderServiceStub;
import io.joynr.android.messaging.binder.util.BinderConstants;
import joynr.system.RoutingTypes.BinderAddress;

/**
 * Stub class that stands in for a bound {@link android.app.Service}. Within it, the 
 * implementation that will create and bind to the {@link android.app.Service} is available.
 */
public class BinderServiceStub implements IBinderServiceStub {

    private Context context;
    private Intent intent;
    private ServiceConnection serviceConnection;
    private BinderAddress toClientAddress;

    public BinderServiceStub(Context context,
                             Intent intent,
                             ServiceConnection serviceConnection,
                             BinderAddress toClientAddress) {
        this.context = context;
        this.intent = intent;
        this.serviceConnection = serviceConnection;
        this.toClientAddress = toClientAddress;
    }

    @Override
    public void createBinderService() {
        BinderServiceSkeleton binderService;
        if (toClientAddress.getUserId() == BinderConstants.USER_ID_SYSTEM) {
            binderService = new BinderServiceServer(context, intent, serviceConnection);
        } else {
            binderService = new BinderServiceClient(context, intent, serviceConnection, toClientAddress);
        }

        binderService.bindService();
    }
}
