#!/bin/bash

# Script para probar deadlock con y sin protocolo de protección

echo "=========================================="
echo "Test de Deadlock en Sistema de Tiempo Real"
echo "=========================================="
echo ""

# Compilar
echo "Compilando el programa..."
gcc -o periodic_sr periodic_sr.c eat.c -lpthread -lrt -Wall
if [ $? -ne 0 ]; then
    echo "Error en compilación"
    exit 1
fi
echo "✓ Compilación exitosa"
echo ""

# Verificar permisos de root
if [ "$EUID" -ne 0 ]; then 
    echo "⚠️  ADVERTENCIA: Este programa requiere permisos de root para usar SCHED_FIFO"
    echo "   Ejecuta: sudo ./test_deadlock.sh"
    echo ""
fi

# Función para ejecutar test
run_test() {
    local protocol=$1
    local timeout=$2
    local description=$3
    
    echo "=========================================="
    echo "$description"
    echo "=========================================="
    echo ""
    
    # Ejecutar con timeout
    timeout $timeout ./periodic_sr &
    PID=$!
    
    # Esperar o timeout
    wait $PID 2>/dev/null
    EXIT_CODE=$?
    
    if [ $EXIT_CODE -eq 124 ]; then
        echo ""
        echo "⏱️  Timeout alcanzado (${timeout}s)"
        echo "   Esto puede indicar deadlock si PROTOCOL=NO"
    elif [ $EXIT_CODE -eq 0 ]; then
        echo ""
        echo "✓ Programa terminó normalmente"
    else
        echo ""
        echo "⚠️  Programa terminó con código: $EXIT_CODE"
    fi
    
    echo ""
}

# Menú interactivo
echo "Selecciona el test a ejecutar:"
echo "1) Test con PROTOCOL=NO  (observar deadlock - 10s timeout)"
echo "2) Test con PROTOCOL=YES (evitar deadlock - 10s timeout)"
echo "3) Ambos tests consecutivos"
echo "4) Salir"
echo ""
read -p "Opción [1-4]: " option

case $option in
    1)
        echo ""
        echo "IMPORTANTE: Verifica que en periodic_sr.c:"
        echo "  const protocol_usage PROTOCOL = NO;"
        echo ""
        read -p "¿Continuar? (s/n): " confirm
        if [ "$confirm" = "s" ]; then
            run_test "NO" "10" "TEST 1: Sin Protocolo (Deadlock Esperado)"
            echo "Si viste que el programa se quedó bloqueado, ¡eso es deadlock!"
        fi
        ;;
    2)
        echo ""
        echo "IMPORTANTE: Verifica que en periodic_sr.c:"
        echo "  const protocol_usage PROTOCOL = YES;"
        echo ""
        read -p "¿Continuar? (s/n): " confirm
        if [ "$confirm" = "s" ]; then
            run_test "YES" "10" "TEST 2: Con Protocolo (Sin Deadlock)"
            echo "El programa debe ejecutarse sin bloqueos permanentes"
        fi
        ;;
    3)
        echo ""
        echo "NOTA: Tendrás que modificar PROTOCOL manualmente entre tests"
        echo "      y recompilar con: gcc -o periodic_sr periodic_sr.c eat.c -lpthread -lrt"
        echo ""
        read -p "Press Enter para continuar..."
        
        echo ""
        echo "1. Primero, configura PROTOCOL = NO y recompila"
        read -p "Press Enter cuando esté listo..."
        run_test "NO" "10" "TEST 1: Sin Protocolo"
        
        echo ""
        echo "2. Ahora, configura PROTOCOL = YES y recompila"
        read -p "Press Enter cuando esté listo..."
        run_test "YES" "10" "TEST 2: Con Protocolo"
        ;;
    4)
        echo "Saliendo..."
        exit 0
        ;;
    *)
        echo "Opción inválida"
        exit 1
        ;;
esac

echo ""
echo "=========================================="
echo "Test completado"
echo "=========================================="
echo ""
echo "Para más información, consulta DEADLOCK_ANALYSIS.md"
