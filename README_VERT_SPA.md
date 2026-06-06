# ESP32_C3_MINI_MORSE_TRAINER  
## Keyer Vertical + Decodificador CW con ESP32-C3 Mini  

**Autor:** LU6APR Pablo Ramos  

---

## 1. DESCRIPCIÓN DEL PROYECTO  

Este proyecto consiste en la construcción de un **Keyer Vertical** (clave Morse tradicional) con decodificador de código Morse integrado, basado en el microcontrolador **ESP32-C3 Super Mini**.  

El dispositivo permite:  

- Transmitir código Morse mediante una **clave vertical** (pulsador único)  
- Decodificar automáticamente el código Morse enviado  
- Mostrar el texto decodificado en una pantalla **OLED (4 líneas)**  
- Ajustar la velocidad de transmisión (WPM)  
- Espaciado automático después de 1 segundo de pausa  
- Borrado automático de pantalla al completar 4 líneas  
- Borrado manual con pulsación corta del botón  

---

## 2. IMAGEN DEL CIRCUITO  

<img width="1971" height="1354" alt="circuit_image" src="https://github.com/user-attachments/assets/04f94726-7019-4d6b-8713-7bc5f921db1e" />  

---

## 3. CONFIGURACIÓN Y CARGA  

1. Copiar el código en el **Arduino IDE**  
2. Seleccionar el puerto correspondiente  
3. Elegir la placa **ESP32-C3 Mini** (en mi caso usé **LOLIN C3 MINI**)  
4. Compilar y subir al ESP32-C3 Mini  

---

## 4. NOTA IMPORTANTE  

El proyecto también puede armarse **sin la pantalla OLED**, funcionando igualmente como **oscilador de código Morse** para clave vertical (straight key).  

---

## 5. DIFERENCIAS CON LA VERSIÓN IÁMBICA  

| Característica       | Versión iámbica (original) | Versión vertical (esta) |
|----------------------|----------------------------|--------------------------|
| Tipo de clave        | Paleta iámbica (2 palancas) | Clave vertical (1 pulsador) |
| Lógica de keyer      | Modo A o B (iámbico)        | Modo recto (presión simple) |
| Decodificación       | Sí                           | Sí                       |
| Ajuste de WPM        | Sí                           | Sí                       |

---

## 6. AUTOR Y LICENCIA  

**LU6APR Pablo Ramos**  
Proyecto de código abierto.  