# Calibra y captura respuestas al escalon del freno, relativas al punto de contacto
import argparse
import time
from pathlib import Path
from banco import Banco
from calibrar import calibrar

DATOS = Path(__file__).resolve().parent.parent / "datos"

p = argparse.ArgumentParser()
p.add_argument("puerto")
p.add_argument("--pwm", type=int, default=150)
p.add_argument("--du", type=int, default=20, help="tamano del escalon en us")
p.add_argument("--antes", type=int, default=0, help="u inicial relativo al contacto")
p.add_argument("--rep", type=int, default=3)
p.add_argument("--nombre", default="escalon")
a = p.parse_args()

DATOS.mkdir(exist_ok=True)
b = Banco(a.puerto)
cal = calibrar(b, a.pwm)
u1 = cal["u_contacto"] + a.antes
u2 = u1 + a.du
print(f"contacto u={cal['u_contacto']}  w0={cal['w0']:.1f} rpm  escalon {u1} -> {u2}")

for k in range(1, a.rep + 1):
    b.enviar(f"u{u1}")
    time.sleep(4)
    b.ser.reset_input_buffer()
    b.enviar(f"e{u2}")
    filas = b.bloque()
    ruta = DATOS / f"{a.nombre}_pwm{a.pwm}_du{a.du}_{k}.csv"
    ruta.write_text("\n".join(filas) + "\n")
    print(f"{ruta.name}: {len(filas) - 1} muestras")

b.cerrar()
