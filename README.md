# Proyecto Grupal 2 ASM: banco de frenado con PID digital

## Estructura
- `firmware/banco/banco.ino`: firmware del Arduino UNO.
- `host/`: scripts de Python para calibrar y capturar datos (requiere `pip install pyserial`).
- `matlab/`: identificación del modelo.
- `datos/`: CSV y calibraciones.

## Conexiones
| Arduino | Conecta a |
|---|---|
| D2 | OUT del sensor KY-033 |
| D5 | ENA del L298 |
| D7 / D8 | IN1 / IN2 del L298 |
| D9 | Señal del servo |

## Comandos seriales (115200, fin de línea `\n`)
| Comando | Acción |
|---|---|
| `m<pwm>` | PWM del motor (0–220) |
| `u<us>` | Pulso del servo en µs |
| `c` | Calibra el freno (motor encendido): imprime `CAL_W0`, `MAP,u,w` y `CAL_FIN,u_contacto,w0` |
| `e<us>` | Escalón del servo: 1 s antes, 6 s después; imprime CSV entre `INICIO` y `FIN` |
| `t<ms>` | Periodo de muestreo (por defecto 20 ms) |
| `f<alfa%>` | Coeficiente del filtro IIR de velocidad (por defecto 30) |
| `x` | Detiene motor y libera el freno |

## Uso
```
python host/calibrar.py COM9 150
python host/capturar.py COM9 --pwm 150 --du 20 --rep 3
```
