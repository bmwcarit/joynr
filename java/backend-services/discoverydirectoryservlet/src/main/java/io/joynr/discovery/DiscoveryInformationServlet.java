package io.joynr.discovery;

import io.joynr.capabilities.CapabilitiesStore;

import java.io.IOException;
import java.io.PrintWriter;

import javax.inject.Singleton;
import javax.servlet.ServletException;
import javax.servlet.annotation.WebServlet;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.inject.Inject;

@Singleton
@WebServlet(value = { "/capabilities/" })
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

    public void destroy() {
        // do nothing.
    }
}