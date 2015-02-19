package io.joynr.discovery;

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

import io.joynr.capabilities.CapabilitiesStore;
import io.joynr.servlet.JoynrWebServlet;

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

@Singleton
@JoynrWebServlet(value = "/capabilities/")
public class DiscoveryInformationServlet extends HttpServlet {
    private static final long serialVersionUID = 8839103126167589803L;
    private transient CapabilitiesStore capabilitiesStore;
    transient private Gson gson = new GsonBuilder().create();

    @Inject
    public DiscoveryInformationServlet(CapabilitiesStore capabilitiesStore) {
        this.capabilitiesStore = capabilitiesStore;
    }

    public void doGet(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
        response.setContentType("text/html");
        PrintWriter out = response.getWriter();
        out.println(gson.toJson(capabilitiesStore.getAllCapabilities()));
    }

    public void doDelete(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
        String queryString = request.getQueryString();
        String[] query = queryString.split("=");
        boolean removed = false;
        if (query.length > 1) {
            removed = capabilitiesStore.remove(query[1]);
        }
        response.setStatus(200);
        PrintWriter out = response.getWriter();
        if (removed) {
            out.println("OK");
        } else {
            out.println("NOK");
        }
    }

    public void destroy() {
        // do nothing.
    }
}