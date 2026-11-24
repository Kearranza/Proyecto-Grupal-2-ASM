# Comunicacion serial con el firmware del banco de frenado
import time
import serial


class Banco:
    def __init__(self, puerto, baud=115200):
        self.ser = serial.Serial(puerto, baud, timeout=2)
        # el Arduino se reinicia al abrir el puerto
        time.sleep(2.5)
        self.ser.reset_input_buffer()

    def enviar(self, cmd, pausa=0.2):
        self.ser.write((cmd + "\n").encode())
        time.sleep(pausa)

    def linea(self):
        return self.ser.readline().decode(errors="ignore").strip()

    def esperar(self, prefijo, limite=120):
        # devuelve la primera linea que empieza con prefijo y las anteriores
        previas, t0 = [], time.time()
        while time.time() - t0 < limite:
            l = self.linea()
            if l.startswith(prefijo):
                return l, previas
            if l:
                previas.append(l)
        raise TimeoutError(f"no llego {prefijo}")

    def bloque(self, limite=60):
        # lineas entre INICIO y FIN, sin comentarios
        self.esperar("INICIO", limite)
        _, lineas = self.esperar("FIN", limite)
        return [l for l in lineas if not l.startswith("#")]

    def cerrar(self):
        self.enviar("x")
        self.ser.close()
