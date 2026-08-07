$certsDir = ".\certs"
if (!(Test-Path $certsDir)) { New-Item -ItemType Directory -Path $certsDir | Out-Null }

Write-Host "Generando Root CA..."
docker run --rm -v ${PWD}\certs:/certs alpine/openssl ecparam -name prime256v1 -genkey -noout -out /certs/ca.key
docker run --rm -v ${PWD}\certs:/certs alpine/openssl req -x509 -new -nodes -key /certs/ca.key -sha256 -days 3650 -out /certs/ca.crt -subj "/C=CO/ST=State/L=City/O=TelemetriaHonda/OU=IT/CN=TelemetriaRootCA"

Write-Host "Generando Certificado Servidor..."
docker run --rm -v ${PWD}\certs:/certs alpine/openssl ecparam -name prime256v1 -genkey -noout -out /certs/server.key
docker run --rm -v ${PWD}\certs:/certs alpine/openssl req -new -key /certs/server.key -out /certs/server.csr -subj "/C=CO/ST=State/L=City/O=TelemetriaHonda/OU=IT/CN=10.162.40.214"
docker run --rm -v ${PWD}\certs:/certs alpine/openssl x509 -req -in /certs/server.csr -CA /certs/ca.crt -CAkey /certs/ca.key -CAcreateserial -out /certs/server.crt -days 3650 -sha256 -extfile /certs/server_ext.cnf -extensions v3_ca

Write-Host "Generando Certificado Cliente (ESP32)..."
docker run --rm -v ${PWD}\certs:/certs alpine/openssl ecparam -name prime256v1 -genkey -noout -out /certs/client_esp32.key
docker run --rm -v ${PWD}\certs:/certs alpine/openssl req -new -key /certs/client_esp32.key -out /certs/client_esp32.csr -subj "/C=CO/ST=State/L=City/O=TelemetriaHonda/OU=Devices/CN=ESP32-Gateway-01"
docker run --rm -v ${PWD}\certs:/certs alpine/openssl x509 -req -in /certs/client_esp32.csr -CA /certs/ca.crt -CAkey /certs/ca.key -CAcreateserial -out /certs/client_esp32.crt -days 3650 -sha256

Write-Host "Certificados generados correctamente en la carpeta certs/"
