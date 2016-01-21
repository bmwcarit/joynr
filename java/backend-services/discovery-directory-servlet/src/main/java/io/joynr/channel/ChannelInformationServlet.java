/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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

package io.joynr.channel;

import java.io.IOException;
import java.io.PrintWriter;

import javax.inject.Singleton;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.inject.Inject;

import io.joynr.servlet.JoynrWebServlet;

@Singleton
@JoynrWebServlet(value = "/channelinfo/")
public class ChannelInformationServlet extends HttpServlet {
    private static final long serialVersionUID = 8839103126167589803L;
    transient private Gson gson = new GsonBuilder().create();
    transient private ChannelUrlDirectoyImpl channelUrlDirectory;

    @Inject
    public ChannelInformationServlet(ChannelUrlDirectoyImpl channelUrlDirectory) {
        this.channelUrlDirectory = channelUrlDirectory;
    }

    @Override
    public void doGet(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
        response.setContentType("text/html");
        PrintWriter out = response.getWriter();
        out.println(gson.toJson(channelUrlDirectory.getRegisteredChannels()));
    }

    @Override
    public void destroy() {
        // do nothing.
    }
}
