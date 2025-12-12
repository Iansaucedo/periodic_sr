# Análisis de la Salida del Programa

## Estado Actual del Sistema

**Configuración**: `PROTOCOL = NO` (configurado para observar deadlock)

---

## Interpretación de la Salida Anterior (con PROTOCOL=YES)

La salida que proporcionaste muestra el sistema ejecutándose **CON** el protocolo de protección activado:

```
1744.348 - Start thread  - 1
...
1.030 - Thread trying to lock R1 - 1
1.030 - Thread acquired R1 - 1
1.055 - Thread trying to lock R2 - 1
1.055 - Thread acquired R2 - 1      ← R2 adquirido inmediatamente
1.070 - Thread trying to lock R2 - 4
1.080 - Thread released R2 - 1
1.080 - Thread released R1 - 1
1.080 - Thread acquired R2 - 4      ← τ₄ espera y luego adquiere R2
...
5.322 - End thread - 4 - 5.312      ← τ₄ completa sin deadlock
```

### Observaciones con PROTOCOL=YES:

1. ✅ **Sin Deadlock**: Los threads completan sus ejecuciones
2. ✅ **Serialización**: τ₁ adquiere ambos mutex, los libera, luego τ₄ los adquiere
3. ✅ **Tiempos de respuesta reportados**: WCRT para todos los threads
4. ⚠️ **Problema de sincronización inicial**: Los threads empezaron en t=1744s (error de sincronización)

---

## Salida Esperada con PROTOCOL=NO (Deadlock)

Ahora que he cambiado a `PROTOCOL = NO`, al ejecutar deberías ver:

```
0.000 - Start thread - 4
0.010 - Start thread - 1
0.050 - Start thread - 2
0.060 - Start thread - 3

# τ₁ comienza primero
0.010 - Thread trying to lock R1 - 1
0.010 - Thread acquired R1 - 1
0.035 - Thread trying to lock R2 - 1    ← τ₁ intenta R2

# Mientras tanto, τ₄ comienza
1.070 - Thread trying to lock R2 - 4    ← τ₄ intenta R2 primero (orden inverso)
1.070 - Thread acquired R2 - 4
1.570 - Thread trying to lock R1 - 4    ← τ₄ intenta R1 (pero τ₁ lo tiene)

[... SILENCIO - SISTEMA BLOQUEADO ...]

🔴 DEADLOCK:
   τ₁: tiene R1, espera R2 (pero τ₄ tiene R2)
   τ₄: tiene R2, espera R1 (pero τ₁ tiene R1)
```

**Indicadores de Deadlock**:

- No más mensajes en la salida
- No aparecen "End thread"
- No se liberan los mutex
- CPU en idle
- Sistema requiere Ctrl+C para terminar

---

## Diferencia Clave: PROTOCOL=YES vs PROTOCOL=NO

### Con PROTOCOL=YES (Priority Ceiling):

```
τ₁ (P=5) intenta R1 → ✓ adquiere R1
τ₁ intenta R2 → ✓ adquiere R2
τ₄ (P=2) intenta R2 → ⏸️ ESPERA (prioridad elevada de τ₁)
τ₁ libera R2 y R1
τ₄ ahora puede adquirir R2 → ✓
τ₄ adquiere R1 → ✓
→ Sin deadlock
```

### Con PROTOCOL=NO (Sin protección):

```
τ₁ (P=5) intenta R1 → ✓ adquiere R1
τ₁ intenta R2 → ⏸️ ESPERA (τ₄ ya lo tiene)
τ₄ (P=2) intenta R2 → ✓ adquiere R2
τ₄ intenta R1 → ⏸️ ESPERA (τ₁ lo tiene)
→ DEADLOCK (ciclo de espera)
```

---

## Cómo Ejecutar la Prueba de Deadlock

```bash
# 1. Verificar configuración actual
cd /home/ian-saucedo/Desktop/periodic_sr
grep "PROTOCOL =" periodic_sr.c
# Debe mostrar: const protocol_usage PROTOCOL = NO;

# 2. Compilar (ya hecho)
gcc -o periodic_sr periodic_sr.c eat.c -lpthread -lrt

# 3. Ejecutar con timeout para prevenir bloqueo infinito
sudo timeout 15s ./periodic_sr

# O ejecutar sin timeout y observar el deadlock:
sudo ./periodic_sr
# Esperar hasta que se detenga la salida (deadlock)
# Presionar Ctrl+C para terminar
```

---

## Resultados del Análisis

### Problema Observado en la Salida Anterior:

**Tiempos iniciales incorrectos** (1744.348 segundos):

- Esto sugiere que `initial_time` no se estaba inicializando correctamente
- Los threads estaban esperando ~29 minutos antes de iniciar
- Sin embargo, el sistema **SÍ funcionaba** (con PROTOCOL=YES)

### Solución Aplicada:

He corregido la inicialización de `data1.wcet1.tv_sec = 0` que faltaba.

### Estado Actual:

- ✅ `PROTOCOL = NO` configurado
- ✅ Código compilado
- ✅ Listo para demostrar deadlock

---

## Próximos Pasos

1. **Ejecutar y observar deadlock**:

   ```bash
   sudo ./periodic_sr
   ```

   - Esperar 1-2 segundos
   - Ver que el sistema se detiene
   - Confirmar deadlock con Ctrl+C

2. **Cambiar a PROTOCOL=YES** para comparar:

   ```bash
   # Editar periodic_sr.c línea 168: PROTOCOL = YES
   gcc -o periodic_sr periodic_sr.c eat.c -lpthread -lrt
   sudo ./periodic_sr
   ```

   - Ver que el sistema continúa ejecutándose
   - Confirmar ausencia de deadlock

3. **Documentar resultados** con capturas de ambos casos

---

## Resumen Visual

### PROTOCOL=NO (Deadlock):

```
t=0.01: τ₁ → [R1 locked]
t=0.04: τ₁ → trying R2... ⏸️
t=1.07: τ₄ → [R2 locked]
t=1.57: τ₄ → trying R1... ⏸️

        🔴 DEADLOCK
        Ninguno puede avanzar
```

### PROTOCOL=YES (Sin Deadlock):

```
t=0.01: τ₁ → [R1 locked, R2 locked]
t=0.08: τ₁ → [Released R2, R1]
t=1.07: τ₄ → [R2 locked, R1 locked]
t=5.32: τ₄ → [Released R1, R2]

        ✅ Sistema funcional
        Ambos completan
```

---

**El sistema está ahora configurado correctamente para demostrar deadlock con `PROTOCOL=NO`.**
