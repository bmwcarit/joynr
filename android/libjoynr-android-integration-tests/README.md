## LibJoynr Integration Tests

The main objective of this project is to test libjoynr on a end to end test scenario. 
In order to implement that the script **test_android_libjoynr.sh** starts: 
- The android emulator;
- The android cluster controller app passing the mqtt broker uri;
- Then starts the test radio provider app;
- And then executes an android instrumentation test on android test radio consumer app where 
there is a test asserting that the provider app using libjoynr returns the correct value 
from the test radio provider app.

Attention: The instrumentation tests inside test-radio-consumer app make no sense individually 
and should only be analyzed inside the integration tests context because those tests won't 
work as they need external dependencies: cluster controller and radio consumer app running.