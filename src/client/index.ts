import mqtt from "mqtt";

const NAMESPACE = "elrojobotones";

const client = mqtt.connect({
    hostname: "65.21.240.153",
    port: 1884,
    clientId: "elrojobotones-js",
});

const $buzzer = document.querySelector("#buzzer") as HTMLAudioElement;
const $connection_status_buttons = document.querySelector(
    "#connection-status-buttons",
) as HTMLSpanElement;
const $connection_status_server = document.querySelector(
    "#connection-status-server",
) as HTMLSpanElement;

const $left_button = document.querySelector(
    "#left-button",
) as HTMLButtonElement;
const $right_button = document.querySelector(
    "#right-button",
) as HTMLButtonElement;

const $buttons = {
    L: $left_button,
    R: $right_button,
};

function sendMessageToButtons(method: string, data: Record<string, any> = {}) {
    client.publish(
        `${NAMESPACE}/callback`,
        JSON.stringify({
            ...data,
            command: "quiz",
            method,
        }),
    );
}

client.on("connect", () => {
    console.log("Connected to MQTT broker");
    $connection_status_server.textContent = "Connected";
    $connection_status_server.classList.add("success");

    client.subscribe(`${NAMESPACE}/availability`);
    client.subscribe(`${NAMESPACE}/quiz`);
    client.subscribe(`${NAMESPACE}/press`);

    // Ping the buttons to check their availability
    client.publish(
        `${NAMESPACE}/callback`,
        JSON.stringify({
            command: "availability",
        }),
    );
});

function whenClientButtonPressed(button: "L" | "R") {
    $buttons[button].classList.add("active");
}

function onReset() {
    sendMessageToButtons("reset");

    $left_button.classList.remove("active");
    $right_button.classList.remove("active");
}

client.on("message", (topic, message) => {
    console.log(`Received message on topic: ${topic}`, message.toString());

    // Handle the received message
    switch (topic) {
        case `${NAMESPACE}/quiz`:
            whenClientButtonPressed(message.toString() as "L" | "R");
            break;

        case `${NAMESPACE}/availability`:
            if (message.toString() === "online") {
                $connection_status_buttons.className = "success";
                $connection_status_buttons.textContent = "Connected";
            } else {
                $connection_status_buttons.className = "error";
                $connection_status_buttons.textContent = "Disconnected";
            }
            break;

        case `${NAMESPACE}/press`:
            $buzzer.currentTime = 0;
            $buzzer.play();
            break;

        default:
            console.log("Unknown topic:", topic);
            break;
    }
});

client.on("error", error => {
    $connection_status_server.textContent = "Errored";
    $connection_status_server.className = "error";
    console.error("Error occurred:", error);
});

document.querySelector("#reset")!.addEventListener("click", onReset);

document.querySelector("#animate")!.addEventListener("click", () => {
    sendMessageToButtons("animation");
});
