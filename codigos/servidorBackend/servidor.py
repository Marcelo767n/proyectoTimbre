import os
import datetime
import requests
from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import urlparse, parse_qs

# ==========================================
# CONFIGURACIÓN (Pon tus datos aquí)
# ==========================================
TELEGRAM_TOKEN = 'TU_TOKEN_DE_BOTFATHER_AQUI'
CHAT_ID = 'TU_CHAT_ID_AQUI' 

def enviar_a_telegram(ruta_imagen, nombre_dispositivo):
    url = f"https://api.telegram.org/bot{TELEGRAM_TOKEN}/sendPhoto"
    caption_text = f"🚨 ¡Alerta de Sistema!\n📍 Origen: {nombre_dispositivo}"
    
    try:
        with open(ruta_imagen, 'rb') as foto:
            datos = {'chat_id': CHAT_ID, 'caption': caption_text}
            archivos = {'photo': foto}
            respuesta = requests.post(url, data=datos, files=archivos)
            
            if respuesta.status_code == 200:
                print(f"✅ Alerta de [{nombre_dispositivo}] entregada en Telegram.")
            else:
                print(f"❌ Error Telegram: {respuesta.text}")
    except Exception as e:
        print(f"❌ Error de envío: {e}")

class TimbreMultiHandler(BaseHTTPRequestHandler):
    def do_POST(self):
        url_parseada = urlparse(self.path)
        
        if url_parseada.path == '/upload':
            parametros = parse_qs(url_parseada.query)
            nombre_dispositivo = parametros.get('id', ['Desconocido'])[0]
            
            longitud = int(self.headers['Content-Length'])
            datos_imagen = self.rfile.read(longitud)
            
            if not os.path.exists('capturas'):
                os.makedirs('capturas')
                
            fecha_hora = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
            nombre_archivo = f"capturas/{nombre_dispositivo}_{fecha_hora}.jpg"
            
            with open(nombre_archivo, 'wb') as f:
                f.write(datos_imagen)
            
            print(f"\n📸 [NUEVA FOTO] Recibida de: {nombre_dispositivo}")
            
            enviar_a_telegram(nombre_archivo, nombre_dispositivo)
            
            self.send_response(200)
            self.send_header('Content-type', 'text/plain')
            self.end_headers()
            self.wfile.write(b"OK")
        else:
            self.send_response(404)
            self.end_headers()

def iniciar_servidor():
    puerto = 8000
    direccion = ('', puerto) 
    servidor = HTTPServer(direccion, TimbreMultiHandler)
    print(f"🚀 Servidor Mecatrónico escuchando en el puerto {puerto}...")
    try:
        servidor.serve_forever()
    except KeyboardInterrupt:
        pass
    servidor.server_close()

if __name__ == '__main__':
    iniciar_servidor()