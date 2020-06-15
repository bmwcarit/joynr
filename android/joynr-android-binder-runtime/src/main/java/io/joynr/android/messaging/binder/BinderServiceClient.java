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
import android.os.UserHandle;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import joynr.system.RoutingTypes.BinderAddress;

/**
 * <p>Class that binds to a particular type of {@link android.app.Service}, the client's. Think of
 * it as the client in a client-server relationship, which in joynr is any app that acts as a client
 * connecting to the server (CC).</p>
 *
 * <p>In a multi-user context, this {@link android.app.Service} is started and bound in a
 * particular user. Note that this uses reflection to use the API-protected
 * {@value METHOD_BIND_SERVICE_AS_USER} method.</p>
 *
 * <p>This kind of {@link android.app.Service} matters when developers want to be multi-user aware,
 * which involves knowledge and use of permissions such as
 * {@code android.permission.INTERACT_ACROSS_USERS},
 * {@code android.permission.INTERACT_ACROSS_USERS_FULL} or {@code android.permission
 * .MANAGE_USERS}.</p>
 * 
 * <p>You can read more on this subject in Android's
 * <a href="https://source.android.com/devices/tech/admin/multiuser-apps">"Building Multiuser-Aware Apps"</a>.</p>
 */
class BinderServiceClient extends BinderServiceSkeleton {

    private static final Logger logger = LoggerFactory.getLogger(BinderServiceClient.class);
    private static final String METHOD_BIND_SERVICE_AS_USER = "bindServiceAsUser";

    private BinderAddress toClientAddress;

    public BinderServiceClient(Context context,
                               Intent intent,
                               ServiceConnection connection,
                               BinderAddress toClientAddress) {
        super(context, intent, connection);
        this.toClientAddress = toClientAddress;
    }

    @Override
    protected void bindService() {
        UserHandle user = UserHandle.getUserHandleForUid(toClientAddress.getUserId());
        try {
            // we need reflection as this is not exposed publicly
            Method method = context.getClass()
                                   .getMethod(METHOD_BIND_SERVICE_AS_USER,
                                              Intent.class,
                                              ServiceConnection.class,
                                              int.class,
                                              UserHandle.class);
            method.invoke(context, intent, connection, Context.BIND_AUTO_CREATE, user);
        } catch (NoSuchMethodException | IllegalAccessException | InvocationTargetException e) {
            logger.error(String.format("Error invoking %1$s, message: %2$s",
                                       METHOD_BIND_SERVICE_AS_USER,
                                       e.getMessage()));
            e.printStackTrace();
        }
    }
}
