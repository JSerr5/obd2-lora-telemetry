import os
from flask import Flask, request, jsonify, send_file
from flask_cors import CORS
import paho.mqtt.publish as publish

app = Flask(__name__)
CORS(app) # Habilitar CORS para el dashboard

# Directorio temporal para el firmware
UPLOAD_FOLDER = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'firmware_update')
FIRMWARE_FILE = os.path.join(UPLOAD_FOLDER, 'firmware.bin')
MQTT_BROKER = "10.162.40.214" # IP del contenedor Mosquitto
MQTT_TOPIC = "esp32/update"

if not os.path.exists(UPLOAD_FOLDER):
    os.makedirs(UPLOAD_FOLDER)

@app.route('/upload', methods=['POST'])
def upload_file():
    if 'firmware' not in request.files:
        return jsonify({"error": "Falta archivo de firmware"}), 400
    file = request.files['firmware']
    if file.filename == '':
        return jsonify({"error": "Archivo vacio"}), 400
    
    file.save(FIRMWARE_FILE)
    print("Firmware recibido y guardado con éxito.")
    return jsonify({"success": True}), 200

@app.route('/trigger', methods=['POST'])
def trigger_update():
    try:
        publish.single(MQTT_TOPIC, "update", hostname=MQTT_BROKER)
        print("Orden OTA enviada por MQTT al Gateway.")
        return jsonify({"success": True}), 200
    except Exception as e:
        print(f"Error enviando MQTT: {e}")
        return jsonify({"error": str(e)}), 500

@app.route('/firmware', methods=['GET'])
def get_firmware():
    if os.path.exists(FIRMWARE_FILE):
        print("El Gateway está descargando el archivo BIN...")
        return send_file(FIRMWARE_FILE, mimetype='application/octet-stream')
    else:
        return jsonify({"error": "Firmware no encontrado"}), 404

if __name__ == '__main__':
    print("Servidor OTA iniciado en el puerto 5000...")
    app.run(host='0.0.0.0', port=5000)
