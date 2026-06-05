from http.server import BaseHTTPRequestHandler, HTTPServer

class Manejador(BaseHTTPRequestHandler):
    def do_POST(self):
        # Leer el tamaño de la foto
        longitud = int(self.headers['Content-Length'])
        datos_imagen = self.rfile.read(longitud)
        
        # Guardar los bytes como un archivo .jpg
        with open("foto_timbre.jpg", "wb") as f:
            f.write(datos_imagen)
            
        print("\n=== ¡NUEVA FOTO RECIBIDA Y GUARDADA! ===")
        print(f"Tamaño: {longitud} bytes")
        
        # Responderle a la ESP32 que todo salió bien
        self.send_response(200)
        self.end_headers()
        self.wfile.write(b"OK")

# Iniciar el servidor en el puerto 8000
print("Servidor Raspberry-Mock escuchando en puerto 8000...")
HTTPServer(('0.0.0.0', 8000), Manejador).serve_forever()