running in jetty:
java -Xdebug -Xnoagent -Djava.compiler=NONE -Xrunjdwp:transport=dt_socket,address=4000,server=y,suspend=n -jar start.jar OPTIONS=Server,jmx etc/jetty-jmx.xml
java -jar start.jar OPTIONS=Server,jmx etc/jetty-jmx.xml

** Create a new channel **
curl -i -X POST -H "Content-type:application/json" -H "X-Atmosphere-tracking-id:123" http://localhost:8080/bounceproxy/channels/?ccid=hello2
curl -i -X POST -H "Content-type:application/json" -H "X-Atmosphere-tracking-id:456" http://localhost:8080/bounceproxy/channels/?ccid=hello456

** Long poll on the channel **
curl -i -X GET -H "Content-type:application/json" -H  "X-Atmosphere-tracking-id:123" http://localhost:8080/bounceproxy/channels/hello2/             
curl -i -X GET -H "Content-type:application/json" -H  "X-Atmosphere-tracking-id:456" http://localhost:8080/bounceproxy/channels/hello456/             

** post a msg to the channel **
curl -i -X POST -H "Content-type:application/json" http://localhost:8080/bounceproxy/channels/hello456/message/ --data "{\"_typeName\":\"joynr.JoynrMessage\",\"header\":{\"expiryDate\":\"1460488123734\",\"msgId\":\"c95cf074-afcc-4416-a9c6-dba2fb9f7e2d\"},\"payload\":\"hello\",\"type\":\"oneWay\"}"
"