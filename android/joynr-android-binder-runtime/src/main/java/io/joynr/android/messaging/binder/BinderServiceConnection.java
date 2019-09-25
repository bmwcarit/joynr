/*-
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
package io.joynr.android.messaging.binder;

import android.content.ComponentName;
import android.content.Context;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.os.RemoteException;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.messaging.FailureAction;
import io.joynr.messaging.SuccessAction;

public class BinderServiceConnection implements ServiceConnection {
    private static final Logger logger = LoggerFactory.getLogger(BinderServiceConnection.class);
    private Context context;
    private byte[] data;
    private SuccessAction successAction;
    private FailureAction failureAction;

    public BinderServiceConnection(Context context, byte[] data, SuccessAction successAction, FailureAction failureAction) {
        this.context = context;
        this.data = data;
        this.successAction = successAction;
        this.failureAction = failureAction;
    }

    public void onServiceConnected(ComponentName componentName, IBinder service) {

        logger.debug("onServiceConnected {}/{}", componentName.getPackageName(), componentName.getShortClassName());
        io.joynr.android.messaging.binder.JoynrBinder remoteServiceClient = io.joynr.android.messaging.binder.JoynrBinder.Stub.asInterface(
                service);
        try {
            remoteServiceClient.transmit(data);
        } catch (RemoteException error) {
            error.printStackTrace();
            failureAction.execute(error);
        }

        context.unbindService(this);
        successAction.execute();
    }

    public void onServiceDisconnected(ComponentName className) {
        logger.error("Service has unexpectedly disconnected remoteServiceClient");
        failureAction.execute(new Throwable("Service has unexpectedly disconnected remoteServiceClient"));
    }
}
