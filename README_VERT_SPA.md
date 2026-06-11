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
- Espaciado automático después de 1 segundo de pausa  
- Borrado manual con pulsación corta del botón  

---

## 2. IMAGEN DEL CIRCUITO  

![Diagrama del Circuito](circuit_image_VERT.png)

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
## 6. CASE
https://www.printables.com/model/1751233-case-cw-trainer-for-vertical-or-iambic-key

## 7. AUTOR Y LICENCIA  

**LU6APR Pablo Ramos**  
Proyecto de código abierto.  
