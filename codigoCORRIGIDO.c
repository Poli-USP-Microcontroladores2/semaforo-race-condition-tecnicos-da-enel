#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <stdio.h>

// Registro do módulo de log
LOG_MODULE_REGISTER(CHECAR_JANTAR, LOG_LEVEL_INF);

// Variável compartilhada
volatile int jantar = 0;

// SEMÁFORO para proteção da variável compartilhada
struct k_sem semaforo_jantar;

// Thread IDs
static k_tid_t chloe_tid, david_tid;

// Declarações das threads
static struct k_thread chloe_thread, david_thread;

// Stack das threads
#define STACK_SIZE 1024
K_THREAD_STACK_DEFINE(chloe_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(david_stack, STACK_SIZE);

// Função do chefe Chloe
void chefe_chloe(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);
    
    while (1) {
        // Pega o semáforo (Down/P/Wait) - Bloqueia se já estiver em uso
        k_sem_take(&semaforo_jantar, K_FOREVER);

        // --- INÍCIO DA SEÇÃO CRÍTICA ---
        int valor_atual = jantar;
        
        // Simula processamento (mantendo o semáforo preso, forçando o outro a esperar)
        k_sleep(K_MSEC(100));
        
        jantar = valor_atual + 1;
        
        LOG_INF("Chloe adicionou sua etapa. Jantar agora: %d", jantar);
        // --- FIM DA SEÇÃO CRÍTICA ---

        // Libera o semáforo (Up/V/Signal)
        k_sem_give(&semaforo_jantar);
        
        // Sleep fora da seção crítica para dar chance ao outro
        k_sleep(K_MSEC(500));
    }
}

// Função do chefe David
void chefe_david(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);
    
    while (1) {
        // Pega o semáforo
        k_sem_take(&semaforo_jantar, K_FOREVER);

        // --- INÍCIO DA SEÇÃO CRÍTICA ---
        int valor_atual = jantar;
        
        k_sleep(K_MSEC(150));
        
        jantar = valor_atual + 1;
        
        LOG_INF("David adicionou sua etapa. Jantar agora: %d", jantar);
        // --- FIM DA SEÇÃO CRÍTICA ---

        // Libera o semáforo
        k_sem_give(&semaforo_jantar);
        
        k_sleep(K_MSEC(500));
    }
}

void main(void)
{
    // Inicializa o semáforo
    // 1 = contagem inicial (livre)
    // 1 = limite máximo (binário/mutex behavior)
    k_sem_init(&semaforo_jantar, 1, 1);

    LOG_INF("Iniciando preparo do jantar...");
    LOG_INF("Jantar inicial: %d", jantar);
    
    chloe_tid = k_thread_create(
        &chloe_thread, chloe_stack, K_THREAD_STACK_SIZEOF(chloe_stack),
        chefe_chloe, NULL, NULL, NULL,
        5, 0, K_NO_WAIT
    );
    
    david_tid = k_thread_create(
        &david_thread, david_stack, K_THREAD_STACK_SIZEOF(david_stack),
        chefe_david, NULL, NULL, NULL,
        5, 0, K_NO_WAIT
    );
    
    LOG_INF("Todas as threads foram criadas!");
    
    while (1) {
        k_sleep(K_SECONDS(10));
    }
}
