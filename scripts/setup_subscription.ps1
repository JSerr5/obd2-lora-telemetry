$ErrorActionPreference = "Stop"

Write-Host "Iniciando configuración de la suscripción en Orion (vía Nginx mTLS usando Docker)..." -ForegroundColor Cyan

$body = '{
  "description": "Notificar a Cygnus sobre cambios en la telemetria para historico",
  "subject": {
    "entities": [
      {
        "idPattern": ".*",
        "type": "Vehicle"
      }
    ],
    "condition": {
      "attrs": [
        "rpm",
        "speed",
        "engineTemp",
        "throttlePosition"
      ]
    }
  },
  "notification": {
    "http": {
      "url": "http://cygnus:5050/notify"
    },
    "attrs": [
      "rpm",
      "speed",
      "engineTemp",
      "throttlePosition"
    ],
    "metadata": ["dateCreated", "dateModified"]
  },
  "throttling": 1
}'

# Pasamos el JSON por stdin a curl dentro del contenedor de Nginx para evitar problemas de comillas
$body | docker exec -i mTLS-nginx-proxy curl -s -iX POST "https://localhost:8443/v2/subscriptions/" --cert /etc/nginx/certs/client_esp32.crt --key /etc/nginx/certs/client_esp32.key --cacert /etc/nginx/certs/ca.crt -H "Content-Type: application/json" "-d" "@-"

Write-Host "`n¡Comando enviado! Si recibes un HTTP 201 Created, la suscripción se creó con éxito." -ForegroundColor Green
