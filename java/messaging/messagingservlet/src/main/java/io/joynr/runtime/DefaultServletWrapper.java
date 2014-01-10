package io.joynr.runtime;

import java.io.IOException;

import javax.inject.Singleton;
import javax.servlet.RequestDispatcher;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

@Singleton
public class DefaultServletWrapper extends HttpServlet {
    private static final long serialVersionUID = 5341721660837975446L;

    public void doGet(HttpServletRequest req, HttpServletResponse resp) throws ServletException, IOException {
        RequestDispatcher requestDispatcher = getServletContext().getNamedDispatcher("default");
        requestDispatcher.forward(req, resp);
    }
}