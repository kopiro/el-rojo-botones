import mqtt from "mqtt";

const client = mqtt.connect({
    hostname: "65.21.240.153",
    port: 1884,
    clientId: "elrojobotones-js",
});

client.on("connect", () => {
    console.log("Connected to MQTT broker");
    // Perform any actions after successful connection
    client.subscribe("elrojobotones/quiz");
});

client.on("message", (topic, message) => {
    console.log(`Received message on topic: ${topic}`);
    console.log(`Message: ${message.toString()}`);

    // Handle the received message
    if (topic === "elrojobotones/quiz") {
        alert(message);
    }
});

client.on("error", error => {
    console.error("Error occurred:", error);
});

document.querySelector("#reset").addEventListener("click", () => {
    client.publish(
        "elrojobotones/callback",
        JSON.stringify({
            command: "quiz",
            method: "reset",
        }),
    );
});

document.querySelector("#animate").addEventListener("click", () => {
    client.publish(
        "elrojobotones/callback",
        JSON.stringify({
            command: "quiz",
            method: "animate",
        }),
    );
});

// Disconnect from the MQTT broker
// client.end();
