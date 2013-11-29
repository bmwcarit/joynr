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
curl -i -X POST -H "Content-type:application/json" http://ajax:9999/bounceproxy/messaging/channels/1234/message/ --data "{\"_typeName\":\"JoynrMessage\",\"type\":\"request\",\"header\":{\"roundTripTtlAbsolute\":\"9360686108031\",\"replyTo\":\"ead4a7af-a724-4ecd-9a4b-1db9b01a897c-end2endA\",\"requestReplyId\":\"c8a9af52-9cd0-4d6b-8ff9-ed4014d46ff8\",\"receiverParticipantID\":\"channelurldirectory_participantid\",\"senderParticipantID\":\"eb9c40d9-af44-4876-871f-fd80c22fb3a5\",\"contentType\":\"application/json\",\"contentClass\":\"Request\"},\"payload\":\"{\\\"_typeName\\\":\\\"Request\\\",\\\"methodName\\\":\\\"getUrlsForChannel\\\",\\\"params\\\":{\\\"channelId\\\":\\\"84afedd5-e822-466a-9377-be57421a05f6-end2endB\\\"},\\\"paramDatatypes\\\":[\\\"java.lang.String\\\"]}\"}"
