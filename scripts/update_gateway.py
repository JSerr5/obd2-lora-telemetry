import os

certs_dir = r"c:\Users\JDSerr\Desktop\VIII\telemetria-esp32\certs"
with open(os.path.join(certs_dir, "ca.crt"), "r") as f: ca_cert = f.read().strip()
with open(os.path.join(certs_dir, "client_esp32.crt"), "r") as f: client_cert = f.read().strip()
with open(os.path.join(certs_dir, "client_esp32.key"), "r") as f: client_key = f.read().strip()

gateway_path = r"c:\Users\JDSerr\Desktop\VIII\telemetria-esp32\src\gateway.cpp"
with open(gateway_path, "r") as f: lines = f.readlines()

new_content = f"""const char *rootCACertificate = R"EOF(
{ca_cert}
)EOF";

const char *clientCertificate = R"EOF(
{client_cert}
)EOF";

const char *clientPrivateKey = R"EOF(
{client_key}
)EOF";

const char *fiware_url_https = "https://10.162.40.214:8443/v2/entities/"
                               "urn:ngsi-ld:Motorcycle:CB300F_Node01/attrs";

// -------- WIFI Y MQTT --------
const char *ssid = "Serrato";
const char *password = "pi-cb300f";
const char *mqtt_server = "10.162.40.214";
const int mqtt_port = 1883;

const char *topic = "/ul/1234/esp32_01/attrs";
const char *topic_ota = "esp32/update";

// -------- OTA --------
const char *firmware_url = "http://10.162.40.214:5000/firmware";
"""

out = []
skip = False
for i, line in enumerate(lines):
    if line.startswith('const char *rootCACertificate = R"EOF('):
        skip = True
        out.append(new_content)
    if skip and line.startswith('const char *firmware_url = '):
        skip = False
        continue
    if not skip:
        out.append(line)

with open(gateway_path, "w") as f:
    f.write("".join(out))
