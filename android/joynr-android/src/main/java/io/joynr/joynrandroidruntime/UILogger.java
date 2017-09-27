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
package io.joynr.joynrandroidruntime;

import java.util.ArrayList;

import android.os.Bundle;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.util.Log;

public class UILogger {
    public static final int MSG_APPEND_LOG = 1;
    public static final int MSG_JOYNR_STARTED = 2;

    ArrayList<Messenger> mClients = new ArrayList<Messenger>();

    public UILogger() {

    }

    public void logText(final String... texts) {
        if (!mClients.isEmpty()) {
            for (String text : texts) {
                sendToAllClients(text);
            }
        }
    }

    public void addLogListener(Messenger clientMessenger) {
        mClients.add(clientMessenger);
    }

    public void removeLogListener(Messenger clientMessanger) {
        mClients.remove(clientMessanger);
    }

    private void sendToAllClients(String text) {
        Bundle bundle = new Bundle();
        bundle.putString("str1", text);
        Message msg = Message.obtain(null, MSG_APPEND_LOG);
        msg.setData(bundle);

        for (Messenger currentClient : mClients) {
            try {
                currentClient.send(msg);
            } catch (RemoteException e) {
                Log.e("JAS", e.getMessage(), e);
                mClients.remove(currentClient);
            }
        }

    }
}
