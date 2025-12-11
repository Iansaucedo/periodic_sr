# Sistema de Threads Periódicos con Deadlock

## Descripción

Este programa demuestra el problema de **deadlock** en sistemas de tiempo real cuando múltiples threads acceden a recursos compartidos en diferente orden, y cómo el **Priority Ceiling Protocol** lo previene.

## Configuración del Sistema

### Threads y Prioridades

| Thread | Prioridad | Período (T) | Cómputo (C) | Recursos | Orden |
| ------ | --------- | ----------- | ----------- | -------- | ----- |
| τ₁     | 5 (Alta)  | 1.4s        | 0.15s       | R1, R2   | R1→R2 |
| τ₂     | 4 (Media) | 2.9s        | 0.6s        | -        | -     |
| τ₃     | 3 (Media) | 13.0s       | 2.7s        | -        | -     |
| τ₄     | 2 (Baja)  | 50.0s       | 5.3s        | R1, R2   | R2→R1 |

### Recursos Compartidos

- **R1 (mutex1)**: Compartido entre τ₁ y τ₄
- **R2 (mutex2)**: Compartido entre τ₁ y τ₄

**⚠️ CLAVE**: τ₁ toma recursos en orden R1→R2, mientras τ₄ los toma en orden R2→R1 (orden inverso).

## Compilación

```bash
gcc -o periodic_sr periodic_sr.c eat.c -lpthread -lrt
```

## Ejecución

El programa requiere privilegios de root para usar scheduling de tiempo real (SCHED_FIFO):

```bash
sudo ./periodic_sr
```

## Configuración del Protocolo

En `periodic_sr.c`, línea ~168:

```c
const protocol_usage PROTOCOL = NO;  // Para observar deadlock
// const protocol_usage PROTOCOL = YES;  // Para evitar deadlock
```

## Pruebas

### Test 1: Sin Protocolo (Observar Deadlock)

1. Configurar `PROTOCOL = NO`
2. Recompilar: `gcc -o periodic_sr periodic_sr.c eat.c -lpthread -lrt`
3. Ejecutar: `sudo ./periodic_sr`

**Resultado Esperado**: El programa se bloqueará mostrando:

```
0.030 - Start thread - 1
0.030 - Thread trying to lock R1 - 1
0.030 - Thread acquired R1 - 1
0.055 - Thread trying to lock R2 - 1
1.070 - Start thread - 4
1.070 - Thread trying to lock R2 - 4
1.070 - Thread acquired R2 - 4
1.570 - Thread trying to lock R1 - 4
[BLOQUEADO - DEADLOCK]
```

- τ₁ tiene R1 y espera R2
- τ₄ tiene R2 y espera R1
- **Dependencia circular → DEADLOCK**

Para terminar el programa bloqueado: `Ctrl+C` o `sudo killall periodic_sr`

### Test 2: Con Protocolo (Evitar Deadlock)

1. Configurar `PROTOCOL = YES`
2. Recompilar: `gcc -o periodic_sr periodic_sr.c eat.c -lpthread -lrt`
3. Ejecutar: `sudo ./periodic_sr`

**Resultado Esperado**: Ejecución normal sin bloqueos:

```
0.030 - Start thread - 1
0.030 - Thread trying to lock R1 - 1
0.030 - Thread acquired R1 - 1
0.055 - Thread trying to lock R2 - 1
0.055 - Thread acquired R2 - 1
0.080 - Thread released R2 - 1
0.080 - Thread released R1 - 1
0.140 - End thread - 1
1.070 - Start thread - 4
...
[Sistema continúa ejecutándose correctamente]
```

El protocolo previene el deadlock mediante el mecanismo de techo de prioridad.

## Script de Prueba Automatizado

```bash
./test_deadlock.sh
```

Este script interactivo te guía a través de los diferentes tests.

## Análisis Detallado

### Cómo Ocurre el Deadlock (PROTOCOL=NO)

```
t=0.000s: τ₁ inicia, ejecuta 0.03s
t=0.030s: τ₁ adquiere R1 ✓
t=0.055s: τ₁ intenta adquirir R2 → BLOQUEADO ⏸️

t=0.010s: τ₄ inicia, ejecuta 1.06s
t=1.070s: τ₄ adquiere R2 ✓
t=1.570s: τ₄ intenta adquirir R1 → BLOQUEADO ⏸️

RESULTADO: Dependencia circular
  τ₁: hold(R1) → wait(R2)
  τ₄: hold(R2) → wait(R1)
  🔴 DEADLOCK
```

### Cómo el Protocolo Evita Deadlock (PROTOCOL=YES)

El **Priority Ceiling Protocol** asigna a cada mutex un "techo" igual a la prioridad más alta de los threads que lo usan (P=5 en este caso).

**Regla**: Un thread solo puede adquirir un mutex si su prioridad es mayor o igual que el techo de todos los mutex actualmente bloqueados.

**Efecto**:

- Cuando τ₄ (P=2) adquiere un mutex, su prioridad efectiva sube a 5
- Esto previene que τ₁ interrumpa a τ₄ mientras tenga recursos
- Los recursos se liberan en orden seguro
- ✓ Sin deadlock

## Distribución de Tiempos

### Thread τ₁ (C=0.15s con recursos)

- `wcet1`: 0.03s (20%) - antes de mutexes
- `wcetmut1` (R1): 0.025s (16.7%)
- `wcetmut2` (R2): 0.025s (16.7%)
- `wcet2`: 0.01s - entre mutexes
- `wcet3`: 0.06s - después de mutexes

### Thread τ₄ (C=5.3s con recursos)

- `wcet1`: 1.06s (20%) - antes de mutexes
- `wcetmut2` (R2): 0.5s (9.4%)
- `wcetmut1` (R1): 0.5s (9.4%)
- `wcet2`: 0.24s - entre mutexes
- `wcet3`: 3.0s - después de mutexes

## Archivos

- `periodic_sr.c` - Programa principal con threads periódicos
- `eat.c` / `eat.h` - Función para simular carga de trabajo
- `timespec_operations.h` - Operaciones con tiempos
- `test_deadlock.sh` - Script de prueba automatizado
- `DEADLOCK_ANALYSIS.md` - Análisis teórico completo
- `README.md` - Esta guía

## Conclusiones

1. **El orden de adquisición de recursos es crítico** - Diferentes órdenes → riesgo de deadlock
2. **Los mutex convencionales no previenen deadlock** - Se requieren protocolos especiales
3. **Priority Ceiling Protocol es efectivo** - Previene deadlock y acota bloqueos
4. **Sistemas de tiempo real requieren análisis cuidadoso** - No basta con "probar" el código
5. **La prevención es mejor que la detección** - El protocolo garantiza ausencia de deadlock

## Referencias

- Sha, Rajkumar, Lehoczky (1990). "Priority Inheritance Protocols"
- Liu & Layland (1973). "Scheduling Algorithms for Multiprogramming"
- POSIX.1-2008 Thread Synchronization Specification

---

**Nota**: Este código es para propósitos educativos y de demostración. En sistemas de producción, siempre usa protocolos de sincronización apropiados (PROTOCOL=YES).
