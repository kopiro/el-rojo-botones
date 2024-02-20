import { app } from "./app";
import http from "http";

http.createServer(app).listen(app.get("port"));
