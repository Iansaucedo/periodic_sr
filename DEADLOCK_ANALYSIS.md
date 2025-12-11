# Análisis de Deadlock con Recursos Compartidos R1 y R2

## Configuración del Sistema

### Recursos Compartidos

- **R1 (mutex1)**: Compartido entre τ₁ y τ₄
- **R2 (mutex2)**: Compartido entre τ₁ y τ₄

### Distribución de Tiempos de Ejecución

#### Thread τ₁ (Alta Prioridad - P=5)

- **Período (T)**: 1.4s
- **Tiempo de Cómputo (C)**: 0.15s
- **Orden de adquisición**: R1 → R2
- **Fase**: 0s (inicia inmediatamente)

Distribución:

- `wcet1`: 0.03s (antes de tomar mutexes) - 20% de C
- `wcetmut1` (R1): 0.025s - 16.7% de C
- `wcetmut2` (R2): 0.025s - 16.7% de C
- `wcet2`: 0.01s (entre liberación de mutexes)
- `wcet3`: 0.06s (después de liberar mutexes)

**Total en mutexes**: 0.05s (33% de C original)

#### Thread τ₄ (Baja Prioridad - P=2)

- **Período (T)**: 50.0s
- **Tiempo de Cómputo (C)**: 5.3s
- **Orden de adquisición**: R2 → R1 ⚠️ **ORDEN INVERSO**
- **Fase**: 0.01s (inicia 10ms después de τ₁)

Distribución:

- `wcet1`: 1.06s (antes de tomar mutexes) - 20% de C
- `wcetmut2` (R2): 0.5s - 9.4% de C
- `wcetmut1` (R1): 0.5s - 9.4% de C
- `wcet2`: 0.24s (entre liberación de mutexes)
- `wcet3`: 3.0s (después de liberar mutexes)

**Total en mutexes**: 1.0s (19% de C original)

---

## Escenario de Deadlock (PROTOCOL=NO)

### Secuencia Temporal que Causa Deadlock

```
Tiempo: 0.000s
│
├─ τ₁ inicia (fase=0)
│  └─ Ejecuta wcet1 (0.03s)
│
├─ 0.010s: τ₄ inicia (fase=0.01s)
│  └─ Ejecuta wcet1 (1.06s)
│
├─ 0.030s: τ₁ termina wcet1
│  └─ Intenta adquirir R1
│     └─ ✓ ADQUIERE R1 (disponible)
│     └─ Ejecuta en R1 (0.025s)
│
├─ 0.055s: τ₁ termina ejecución en R1
│  └─ Intenta adquirir R2 (mientras mantiene R1)
│     └─ ⏸️ BLOQUEADO esperando R2
│
├─ 1.070s: τ₄ termina wcet1
│  └─ Intenta adquirir R2 (su primer mutex)
│     └─ ✓ ADQUIERE R2 (disponible)
│     └─ Ejecuta en R2 (0.5s)
│
├─ 1.570s: τ₄ termina ejecución en R2
│  └─ Intenta adquirir R1 (mientras mantiene R2)
│     └─ ⏸️ BLOQUEADO esperando R1
│
├─ **DEADLOCK DETECTADO**
│  ├─ τ₁ tiene R1, espera R2
│  └─ τ₄ tiene R2, espera R1
│
└─ 🔴 SISTEMA BLOQUEADO - Dependencia circular
```

### Diagrama de Estado del Deadlock

```
    τ₁ (P=5)                        τ₄ (P=2)
    ┌──────┐                        ┌──────┐
    │ Hold │                        │ Hold │
    │  R1  │                        │  R2  │
    └──┬───┘                        └──┬───┘
       │                               │
       │ Waiting for                   │ Waiting for
       │    R2                          │    R1
       │                               │
       ▼                               ▼
    ┌──────┐                        ┌──────┐
    │ Need │◄───── Circular ───────►│ Need │
    │  R2  │      Dependency        │  R1  │
    └──────┘                        └──────┘
```

### Condiciones Necesarias para Deadlock (Todas presentes)

1. ✓ **Exclusión Mutua**: Los recursos R1 y R2 no pueden ser compartidos simultáneamente
2. ✓ **Hold and Wait**: Los threads mantienen un recurso mientras esperan otro
3. ✓ **No Preemption**: Los recursos no pueden ser arrebatados forzosamente
4. ✓ **Circular Wait**: τ₁ espera a τ₄ y τ₄ espera a τ₁

---

## Solución: Priority Ceiling Protocol (PROTOCOL=YES)

### Cómo Evita el Deadlock

El protocolo de techo de prioridad (Priority Ceiling Protocol) asigna a cada mutex un "techo de prioridad" igual a la prioridad más alta de todos los threads que pueden bloquearlo.

#### Configuración con Protocolo

```c
const protocol_usage PROTOCOL = YES;
pthread_mutexattr_setprotocol(&mutexattr1, PTHREAD_PRIO_PROTECT);
pthread_mutexattr_setprioceiling(&mutexattr1, sched_get_priority_min(SCHED_FIFO) + 5);
```

**Techo de prioridad**: P=5 (prioridad de τ₁)

### Mecanismo de Prevención

Cuando un thread intenta adquirir un mutex con protocolo de techo:

1. **Regla**: Un thread solo puede adquirir un mutex si su prioridad es MAYOR que el techo de prioridad de todos los mutex actualmente bloqueados por otros threads.

2. **En nuestro caso**:
   - Cuando τ₄ (P=2) adquiere R2 (techo=5), su prioridad efectiva sube a 5
   - Cuando τ₁ (P=5) intenta adquirir R1, puede proceder porque su prioridad (5) es igual al techo
   - τ₄ con prioridad elevada NO será interrumpido hasta liberar sus recursos

### Secuencia con Protocolo Activo

```
Tiempo: 0.000s
│
├─ τ₁ inicia (fase=0)
│  └─ Ejecuta wcet1 (0.03s)
│
├─ 0.010s: τ₄ inicia (fase=0.01s)
│  └─ Ejecuta wcet1 (1.06s)
│
├─ 0.030s: τ₁ termina wcet1
│  └─ Intenta adquirir R1
│     └─ ✓ ADQUIERE R1 con techo P=5
│     └─ Ejecuta en R1 (0.025s)
│
├─ 0.055s: τ₁ termina ejecución en R1
│  └─ Intenta adquirir R2
│     └─ ⏸️ BLOQUEADO porque τ₄ está activo
│        (τ₄ podría intentar tomar R2 y crear conflicto)
│
├─ 1.070s: τ₄ termina wcet1
│  └─ Intenta adquirir R2
│     └─ ❌ BLOQUEADO por el protocolo
│        (No puede adquirir R2 mientras τ₁ tiene R1)
│        └─ Prioridad de τ₄ < techo de R2 cuando R1 está bloqueado
│
├─ τ₁ eventualmente adquiere R2 primero
│  └─ Completa ambas secciones críticas
│  └─ Libera R1 y R2
│
├─ Ahora τ₄ puede adquirir R2 luego R1
│  └─ ✓ Sin deadlock
│
└─ ✅ SISTEMA FUNCIONAL
```

### Beneficios del Protocolo

1. **Previene Deadlock**: Rompe la condición de "circular wait"
2. **Acota Bloqueo**: El tiempo máximo de bloqueo está acotado
3. **Predecible**: Permite análisis de schedulability
4. **Evita Inversión de Prioridad Prolongada**: Los threads de baja prioridad heredan prioridad alta

---

## Compilación y Ejecución

### Sin Protocolo (observar deadlock)

```bash
# En periodic_sr.c, asegurar: const protocol_usage PROTOCOL = NO;
gcc -o periodic_sr periodic_sr.c eat.c -lpthread -lrt
sudo ./periodic_sr
```

**Resultado esperado**: El programa se bloqueará mostrando mensajes como:

```
0.030 - Thread trying to lock R1 - 1
0.030 - Thread acquired R1 - 1
0.055 - Thread trying to lock R2 - 1
1.070 - Thread trying to lock R2 - 4
1.070 - Thread acquired R2 - 4
1.570 - Thread trying to lock R1 - 4
[SISTEMA BLOQUEADO - DEADLOCK]
```

### Con Protocolo (evitar deadlock)

```bash
# En periodic_sr.c, cambiar a: const protocol_usage PROTOCOL = YES;
gcc -o periodic_sr periodic_sr.c eat.c -lpthread -lrt
sudo ./periodic_sr
```

**Resultado esperado**: Ejecución normal sin bloqueos permanentes:

```
0.030 - Start thread - 1
0.030 - Thread trying to lock R1 - 1
0.030 - Thread acquired R1 - 1
0.055 - Thread trying to lock R2 - 1
0.055 - Thread acquired R2 - 1
0.080 - Thread released R2 - 1
0.080 - Thread released R1 - 1
0.140 - End thread - 1 - 0.110
...
[Sistema continúa ejecutándose correctamente]
```

---

## Análisis de Resultados

### Observaciones Sin Protocolo (PROTOCOL=NO)

1. **Deadlock Confirmado**:

   - τ₁ adquiere R1 primero
   - τ₄ adquiere R2 primero
   - Ambos quedan esperando el recurso del otro indefinidamente

2. **Síntomas**:

   - CPU idle (no hay progreso)
   - Threads en estado BLOCKED permanente
   - Último mensaje: threads intentando adquirir mutex

3. **Impacto**:
   - Sistema completamente inutilizable
   - Requiere kill forzado del proceso
   - Ningún thread puede completar su trabajo

### Observaciones Con Protocolo (PROTOCOL=YES)

1. **Deadlock Evitado**:

   - El protocolo serializa el acceso a recursos
   - Los threads completan sus secciones críticas
   - El sistema continúa funcionando

2. **Comportamiento**:

   - Aumento temporal de prioridad efectiva
   - Bloqueos acotados y predecibles
   - Todos los threads eventualmente progresan

3. **Trade-offs**:
   - Mayor overhead por gestión de prioridades
   - Serialización puede reducir concurrencia
   - Pero garantiza ausencia de deadlock

---

## Conclusiones

1. **Orden de Adquisición Importa**: Cuando dos threads adquieren múltiples recursos en diferente orden, el riesgo de deadlock es muy alto.

2. **Protocolo de Protección es Esencial**: En sistemas de tiempo real, el Priority Ceiling Protocol no solo previene deadlock, sino que también garantiza límites superiores en tiempos de respuesta.

3. **Análisis Estático Posible**: Con el protocolo, podemos calcular matemáticamente el peor caso sin necesidad de ejecutar exhaustivamente.

4. **Diseño Crítico**: La configuración correcta de fases y orden de recursos puede exponer o evitar problemas de concurrencia.

5. **Verificación Práctica**: Esta demostración muestra empíricamente por qué los sistemas de tiempo real críticos SIEMPRE deben usar protocolos de sincronización apropiados.

---

## Referencias

- Liu, C. L., & Layland, J. W. (1973). "Scheduling Algorithms for Multiprogramming in a Hard-Real-Time Environment"
- Sha, L., Rajkumar, R., & Lehoczky, J. P. (1990). "Priority Inheritance Protocols: An Approach to Real-Time Synchronization"
- POSIX.1-2008: IEEE Std 1003.1-2008 - Thread Synchronization
