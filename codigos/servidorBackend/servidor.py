import os
import datetime
from http.server import BaseHTTPRequestHandler, HTTPServer

class Manejador(BaseHTTPRequestHandler):
    def do_POST(self):
        longitud = int(self.headers['Content-Length'])
        datos_imagen = self.rfile.read(longitud)
        
        # Crear nombre de archivo único con la fecha y hora
        fecha_hora = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
        nombre_archivo = f"foto_timbre_{fecha_hora}.jpg"
        
        # Asegurar que la carpeta 'capturas' exista
        if not os.path.exists("capturas"):
            os.makedirs("capturas")
            
        ruta_completa = os.path.join("capturas", nombre_archivo)
        
        with open(ruta_completa, "wb") as f:
            f.write(datos_imagen)
            
        print(f"\n=== ¡FOTO GUARDADA! === -> {ruta_completa}")
        
        self.send_response(200)
        self.send_header('Content-Length', '2')
        self.end_headers()
        self.wfile.write(b"OK")

print("Servidor escuchando en puerto 8000...")
HTTPServer(('0.0.0.0', 8000), Manejador).serve_forever()