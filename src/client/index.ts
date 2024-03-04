import mqtt from "mqtt";

const NAMESPACE = "elrojobotones";

const client = mqtt.connect({
    hostname: "65.21.240.153",
    port: 1884,
    clientId: "elrojobotones-js",
});

const $buzzer_left = document.querySelector("#buzzer-left") as HTMLAudioElement;
const $buzzer_right = document.querySelector(
    "#buzzer-right",
) as HTMLAudioElement;

const $connection_status_buttons = document.querySelector(
    "#connection-status-buttons",
) as HTMLSpanElement;
const $connection_status_server = document.querySelector(
    "#connection-status-server",
) as HTMLSpanElement;
const $sentence = document.querySelector("#sentence") as HTMLDivElement;

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
    $connection_status_server.textContent = "Connected";
    $connection_status_server.classList.add("success");

    client.subscribe(`${NAMESPACE}/availability`);
    client.subscribe(`${NAMESPACE}/quiz`);
    client.subscribe(`${NAMESPACE}/press`);

    // Ping the buttons to check their availability
    // client.publish(
    //     `${NAMESPACE}/callback`,
    //     JSON.stringify({
    //         command: "availability",
    //     }),
    // );
});

const score = {
    L: 0,
    R: 0,
};

let intv: ReturnType<typeof setTimeout> | null = null;
let quizStarted = false;
let pressIntv: ReturnType<typeof setTimeout> | null = null;

function whenClientButtonPressed(button: "L" | "R") {
    if (intv) clearTimeout(intv);

    if (quizStarted) {
        score[button]++;
        $buttons[button].classList.add("success");
        $buttons[button == "L" ? "R" : "L"].classList.add("late");
    } else {
        score[button] -= 2;
        $buttons[button].classList.add("wrong");
        $buttons[button == "L" ? "R" : "L"].classList.add("other_is_wrong");
    }

    for (const key in $buttons) {
        $buttons[key]!.textContent = score[key];
    }

    setTimeout(() => {
        onReset();
    }, 3000);
}

function onReset() {
    quizStarted = false;
    sendMessageToButtons("reset");

    $left_button.className = "button";
    $right_button.className = "button";

    // Generate random character in the sentence
    const text = [];
    for (let i = 0; i < 500; i++) {
        text.push(String.fromCharCode(65 + Math.floor(Math.random() * 26)));
    }
    $sentence.textContent = text.join(" ");

    if (intv) clearTimeout(intv);
    if (pressIntv) clearTimeout(pressIntv);
    intv = setTimeout(() => {
        quizStarted = true;

        pressIntv = setTimeout(() => {
            onReset();
        }, 3_000);

        // Change a random letter in s $sentence
        const index = Math.floor(Math.random() * text.length);
        text[index] = String.fromCharCode(65 + Math.floor(Math.random() * 26));
        $sentence.textContent = text.join(" ");
    }, Math.random() * 10_000);
}
``;
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
            if (message.toString() === "L") {
                $buzzer_left.currentTime = 0;
                $buzzer_left.play();
            } else if (message.toString() === "R") {
                $buzzer_right.currentTime = 0;
                $buzzer_right.play();
            }
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
