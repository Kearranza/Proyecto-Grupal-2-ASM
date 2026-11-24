# Calibra el freno: punto de contacto, velocidad sin freno y mapa estatico u -> w
import json
import sys
import time
from pathlib import Path
from banco import Banco

DATOS = Path(__file__).resolve().parent.parent / "datos"


def calibrar(b, pwm, espera=6):
    b.enviar(f"m{pwm}")
    time.sleep(espera)
    b.ser.reset_input_buffer()
    b.enviar("c")
    fin, lineas = b.esperar("CAL_FIN")
    mapa = [tuple(map(float, l.split(",")[1:])) for l in lineas if l.startswith("MAP,")]
    _, u0, w0 = fin.split(",")
    return {"pwm": pwm, "u_contacto": int(u0), "w0": float(w0), "mapa": mapa}


if __name__ == "__main__":
    # uso: python calibrar.py COM9 [pwm]
    puerto = sys.argv[1]
    pwm = int(sys.argv[2]) if len(sys.argv) > 2 else 150
    b = Banco(puerto)
    cal = calibrar(b, pwm)
    b.cerrar()
    DATOS.mkdir(exist_ok=True)
    ruta = DATOS / f"calibracion_pwm{pwm}.json"
    ruta.write_text(json.dumps(cal, indent=2))
    print(f"contacto u={cal['u_contacto']}  w0={cal['w0']:.1f} rpm  ({len(cal['mapa'])} puntos) -> {ruta.name}")
