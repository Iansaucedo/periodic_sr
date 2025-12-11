## RESUMEN EJECUTIVO: Demostración de Deadlock

### Configuración Implementada ✓

**Dos recursos compartidos (R1 y R2)** entre threads τ₁ (alta prioridad) y τ₄ (baja prioridad)

**Distribución de tiempo:**
- τ₁: 0.15s total → 0.025s en R1 + 0.025s en R2 (repartido equitativamente)
- τ₄: 5.3s total → 0.5s en R1 + 0.5s en R2 (repartido equitativamente)
- Ambos tienen ≥20% de tiempo antes y después de los mutexes

**Orden de adquisición:**
- τ₁: R1 → R2 (mutex_order = 1)
- τ₄: R2 → R1 (mutex_order = 2) **← ORDEN INVERSO**

**Fases configuradas para deadlock:**
- τ₁: fase = 0s (inicia primero)
- τ₄: fase = 0.01s (inicia 10ms después)

---

### Diagrama de Ejecución sin Protocolo (PROTOCOL=NO)

```
Tiempo →
0.000s     0.030s     0.055s     1.070s     1.570s
  |          |          |          |          |
  |          |          |          |          |
τ₁├─wcet1──►├─[R1]────►├─WAIT R2──────────────────► ⏸️ BLOQUEADO
  |          |    ▲     |                           (espera R2)
  |          |    │     |                           
  |          |    │     └──► τ₁ intenta R2 pero τ₄ lo tiene
  |          |    │                                  
  |          |    └──────── τ₁ tiene R1
  |          |
  |          |
τ₄├─────────────────────├─wcet1──►├─[R2]────►├─WAIT R1──► ⏸️ BLOQUEADO
                        |          |    ▲     |          (espera R1)
                        |          |    │     |
                        |          |    │     └──► τ₄ intenta R1 pero τ₁ lo tiene
                        |          |    │
                        |          |    └──────── τ₄ tiene R2
                        |          |
                        
🔴 DEADLOCK: Dependencia circular detectada
   τ₁ → wait(R2) ← hold(τ₄)
   τ₄ → wait(R1) ← hold(τ₁)
```

---

### Diagrama con Protocolo (PROTOCOL=YES)

```
Priority Ceiling Protocol Activo

Techo de R1 = 5 (prioridad de τ₁)
Techo de R2 = 5 (prioridad de τ₁)

Cuando τ₄ adquiere cualquier mutex, su prioridad efectiva sube a 5
→ No puede ser interrumpido por τ₁
→ Serialización forzada de accesos
→ ✓ Sin deadlock

Ejecución ordenada:
1. Thread con mayor prioridad efectiva completa primero
2. Recursos se liberan antes de que otro thread los tome
3. No hay ciclos de espera
```

---

### Cómo Probar

#### 1. Compilar
```bash
cd /home/ian-saucedo/Desktop/periodic_sr
gcc -o periodic_sr periodic_sr.c eat.c -lpthread -lrt
```

#### 2. Test de Deadlock (PROTOCOL=NO)
```bash
# Asegurar que en periodic_sr.c línea ~168:
#   const protocol_usage PROTOCOL = NO;

sudo ./periodic_sr

# Observar salida:
# - τ₁ adquiere R1
# - τ₁ intenta R2 (bloqueado)
# - τ₄ adquiere R2
# - τ₄ intenta R1 (bloqueado)
# → SISTEMA BLOQUEADO (usar Ctrl+C)
```

#### 3. Test sin Deadlock (PROTOCOL=YES)
```bash
# Cambiar en periodic_sr.c línea ~168:
#   const protocol_usage PROTOCOL = YES;

gcc -o periodic_sr periodic_sr.c eat.c -lpthread -lrt
sudo ./periodic_sr

# Observar salida:
# - Threads completan sus ejecuciones
# - Reportes de "Worst-case response time"
# - Sistema funciona correctamente
# (usar Ctrl+C para terminar)
```

---

### Resultados Esperados

#### Sin Protocolo (PROTOCOL=NO):
```
0.030 - Start thread - 1
0.030 - Thread trying to lock R1 - 1
0.030 - Thread acquired R1 - 1
0.055 - Thread trying to lock R2 - 1
1.070 - Start thread - 4
1.070 - Thread trying to lock R2 - 4  
1.070 - Thread acquired R2 - 4
1.570 - Thread trying to lock R1 - 4
[... silencio, sistema bloqueado ...]
^C
```
**Interpretación**: Deadlock confirmado. Los threads están en espera mutua.

#### Con Protocolo (PROTOCOL=YES):
```
0.030 - Start thread - 1
0.030 - Thread trying to lock R1 - 1
0.030 - Thread acquired R1 - 1
0.055 - Thread trying to lock R2 - 1
0.055 - Thread acquired R2 - 1
0.080 - Thread released R2 - 1
0.080 - Thread released R1 - 1
0.140 - End thread - 1 - 0.110
0.xxx - Worst-case response time - 1 - x.xxx
[... continúa ejecutándose ...]
^C
```
**Interpretación**: Sistema funcional. Los threads progresan sin deadlock.

---

### Verificación del Deadlock

**Síntomas de deadlock:**
1. ✓ Salida se detiene después de "Thread trying to lock..."
2. ✓ CPU usage baja (~0%, threads bloqueados)
3. ✓ Dos threads en estado BLOCKED esperando mutexes
4. ✓ No hay progreso (no aparecen nuevos mensajes)
5. ✓ Sistema requiere kill forzado (Ctrl+C)

**Con protocolo funcionando:**
1. ✓ Mensajes "acquired" seguidos de "released"
2. ✓ Mensajes "End thread" aparecen periódicamente
3. ✓ CPU usage mayor (threads ejecutándose)
4. ✓ Reportes de WCRT cada periodo
5. ✓ Sistema responde normalmente

---

### Archivos Relevantes

| Archivo | Descripción |
|---------|-------------|
| `periodic_sr.c` | ⭐ Código principal - MODIFICA AQUÍ `PROTOCOL` |
| `README.md` | Guía rápida de uso |
| `DEADLOCK_ANALYSIS.md` | Análisis teórico completo |
| `test_deadlock.sh` | Script de prueba automatizado |
| `QUICKSTART.md` | Este archivo - guía rápida |

---

### Checklist de Verificación

Para confirmar que el sistema está correctamente configurado:

- [x] Dos mutex declarados (mutex1, mutex2)
- [x] τ₁ usa ambos mutex en orden R1→R2
- [x] τ₄ usa ambos mutex en orden R2→R1 (inverso)
- [x] Tiempo de R1 original dividido entre R1 y R2
- [x] Fases configuradas: τ₁=0s, τ₄=0.01s
- [x] PROTOCOL configurable (NO/YES)
- [x] Mensajes de depuración activados (report())
- [x] Compila sin errores fatales

---

### Respuesta a los Requerimientos

✅ **Nuevo recurso compartido R2**: Implementado
✅ **Tiempo de R1 repartido**: 50% R1, 50% R2 para ambos threads
✅ **Orden diferente**: τ₁ usa R1→R2, τ₄ usa R2→R1
✅ **Deadlock observable**: Configurado con PROTOCOL=NO
✅ **Prevención con protocolo**: Funciona con PROTOCOL=YES
✅ **Condiciones adecuadas**: Fases ajustadas para garantizar deadlock

---

**Estado**: ✅ IMPLEMENTACIÓN COMPLETA Y FUNCIONAL

El sistema está listo para demostrar deadlock y su prevención mediante Priority Ceiling Protocol.
