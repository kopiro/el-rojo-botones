import mqtt from "mqtt";

const client = mqtt.connect({
    hostname: "65.21.240.153",
    port: 1884,
    // Clean session
    clean: true,
    connectTimeout: 4000,
    // Authentication
    clientId: "elrojobotones-js",
});

client.on("connect", () => {
    console.log("Connected to MQTT broker");
    // Perform any actions after successful connection

    client.subscribe("elrojobotones/availability", function (err) {
        console.log("READY");
    });
});

client.on("message", (topic, message) => {
    console.log(`Received message on topic: ${topic}`);
    console.log(`Message: ${message.toString()}`);
    // Handle the received message
});

client.on("error", error => {
    console.error("Error occurred:", error);
    // Handle the error
});

document.querySelector("#test").addEventListener("click", () => {
    const z = client.publish(
        "elrojobotones/callback",
        JSON.stringify({
            command: "quiz",
        }),
    );
    console.log(z);
});

// Disconnect from the MQTT broker
// client.end();
